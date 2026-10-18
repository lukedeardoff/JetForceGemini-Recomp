#include "jfg/runtime/input_replay.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

int failures = 0;

void check(const bool condition, const std::string_view message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

class TemporaryReplay final {
public:
    explicit TemporaryReplay(const std::string_view contents) {
        static std::atomic<std::uint64_t> sequence{0U};
        const auto nonce =
            std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
            ("jfg-input-replay-" + std::to_string(nonce) + "-" +
             std::to_string(sequence.fetch_add(1U)) + ".txt");
        std::ofstream stream(path_, std::ios::binary);
        stream.write(contents.data(),
                     static_cast<std::streamsize>(contents.size()));
        if (!stream) {
            throw std::runtime_error("failed to write input replay fixture");
        }
    }

    ~TemporaryReplay() noexcept {
        std::error_code ignored;
        std::filesystem::remove(path_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }

private:
    std::filesystem::path path_;
};

void test_valid_replay() {
    const TemporaryReplay fixture(
        "jfg-phase8-input-v1\r\n"
        "1800,1810,1000,0,0\r\n"
        "2050,2060,8000,-12,34\r\n");
    jfg::DeterministicInputReplay replay;
    check(jfg::DeterministicInputReplay::load(fixture.path(), replay) ==
              jfg::InputReplayError::none,
          "valid replay should load");
    check(replay.events().size() == 2U, "valid replay event count");
    check(replay.sample_at(1799U).neutral(), "pre-event sample is neutral");
    check(replay.sample_at(1800U).buttons == 0x1000U,
          "first boundary is inclusive");
    check(replay.sample_at(1809U).buttons == 0x1000U,
          "event remains active before end");
    check(replay.sample_at(1810U).neutral(), "end boundary is exclusive");
    check(replay.sample_at(2055U) ==
              jfg::ControllerReplaySample{true, 0x8000U, -12, 34},
          "later event preserves buttons and stick values");
}

void test_connection_transitions() {
    const TemporaryReplay fixture(
        "jfg-phase8-input-v2\n"
        "100,110,0,0000,0,0\n"
        "120,130,1,8000,12,-34\n");
    jfg::DeterministicInputReplay replay;
    check(jfg::DeterministicInputReplay::load(fixture.path(), replay) ==
              jfg::InputReplayError::none,
          "connection replay should load");
    check(!replay.sample_at(105U).connected,
          "disconnect window is observable");
    check(replay.sample_at(110U).connected,
          "controller reconnects after disconnect window");
    check(replay.sample_at(125U) ==
              jfg::ControllerReplaySample{true, 0x8000U, 12, -34},
          "connected v2 sample preserves controls");
}

void test_poll_replay_eof_is_neutral() {
    const TemporaryReplay fixture(
        "jfg-phase8-input-v2\n"
        "0,1,1,0000,0,0\n"
        "1,100,1,8000,12,-34\n");
    jfg::DeterministicInputReplay replay;
    check(jfg::DeterministicInputReplay::load(fixture.path(), replay) ==
              jfg::InputReplayError::none,
          "poll replay should load");
    check(replay.sample_by_poll(0U).neutral(), "first poll is neutral");
    check(replay.sample_by_poll(1U) ==
              jfg::ControllerReplaySample{true, 0x8000U, 12, -34},
          "last recorded poll preserves buttons and stick");
    check(replay.sample_by_poll(2U).neutral(),
          "first poll after EOF is neutral despite long final interval");
    check(replay.sample_by_poll(100U).neutral(),
          "later polls after EOF remain neutral");
}

void test_long_manual_recording() {
    std::string contents = "jfg-phase8-input-v2\n";
    for (std::size_t i = 0U; i < jfg::kMaximumInputReplayEvents; ++i) {
        contents += std::to_string(i * 2U) + "," +
                    std::to_string(i * 2U + 1U) + ",1,8000,12,-34\n";
    }
    check(contents.size() > 1024U * 1024U,
          "manual recording exceeds the former one MiB limit");
    jfg::DeterministicInputReplay replay;
    const TemporaryReplay valid(contents);
    check(jfg::DeterministicInputReplay::load(valid.path(), replay) ==
              jfg::InputReplayError::none, "long manual recording loads");
    check(replay.sample_by_poll(jfg::kMaximumInputReplayEvents - 1U) ==
              jfg::ControllerReplaySample{true, 0x8000U, 12, -34},
          "final input of long recording remains intact");
    contents += "900000,900001,1,0,0,0\n";
    const TemporaryReplay excess(contents);
    check(jfg::DeterministicInputReplay::load(excess.path(), replay) ==
              jfg::InputReplayError::invalid_record,
          "event limit still rejects oversized event counts");
    const TemporaryReplay too_large(std::string(8U * 1024U * 1024U + 1U, 'x'));
    check(jfg::DeterministicInputReplay::load(too_large.path(), replay) ==
              jfg::InputReplayError::too_large, "byte limit remains enforced");
}

void test_independent_look() {
    const TemporaryReplay valid("jfg-phase8-input-v3\n100,120,1,2010,80,-40,3,180,-90\n");
    jfg::DeterministicInputReplay replay;
    check(jfg::DeterministicInputReplay::load(valid.path(),replay)==jfg::InputReplayError::none,"v3 independent look loads");
    const auto sample=replay.sample_at(105);
    check(sample.stick_x==80 && sample.stick_y==-40 && sample.look_x==180 && sample.look_y==-90 && sample.pc_mode==3,"movement and mouse look stay independent");
    check(replay.sample_at(120).pc_mode==0 && replay.sample_at(120).neutral(),"look ends at event boundary");
    const auto before=replay.events();
    for(const auto* row:{"1,2,1,0,0,0,5,0,0","1,2,1,0,0,0,0,1,0",
        "1,2,0,0,0,0,3,1,0","1,2,1,0,0,0,2,128,0","1,2,1,0,0,0,3,32768,0",
        "1,2,1,0,0,0,3,2","1,2,1,0,0,0,3,1,2,3"}) {
      const TemporaryReplay bad(std::string("jfg-phase8-input-v3\n")+row+"\n");
      check(jfg::DeterministicInputReplay::load(bad.path(),replay)==jfg::InputReplayError::invalid_record && replay.events()==before,"invalid v3 leaves active replay intact");
    }
}


void test_modern_action_recording() {
    const TemporaryReplay valid("jfg-phase8-input-v5\n100,120,1,2010,80,-40,5,6144,-18432,129\n120,121,1,8000,0,0,0,0,0,128\n");
    jfg::DeterministicInputReplay replay;
    check(jfg::DeterministicInputReplay::load(valid.path(),replay)==jfg::InputReplayError::none,"v5 modern actions load");
    check(replay.sample_at(110).modern_flags==129 && replay.sample_at(110).look_y==-18432,"v5 scope commands and precise look survive");
    check(replay.sample_at(120).modern_flags==128 && replay.sample_at(120).buttons==0x8000,"modern menus do not require captured mouse look");
    check(replay.sample_at(121).neutral(),"modern actions release at EOF");
    const auto before=replay.events();
    for(const auto *row:{"1,2,1,0,0,0,0,0,0,1","1,2,1,0,0,0,0,0,0,136",
        "1,2,0,0,0,0,0,0,0,128","1,2,1,0,0,0,0,0,0,256",
        "1,2,1,0,0,0,0,0,0","1,2,1,0,0,0,0,0,0,128,0"}) {
        const TemporaryReplay bad(std::string("jfg-phase8-input-v5\n")+row+"\n");
        check(jfg::DeterministicInputReplay::load(bad.path(),replay)==jfg::InputReplayError::invalid_record && replay.events()==before,"invalid modern flags reject atomically");
    }
    const TemporaryReplay old("jfg-phase8-input-v4\n1,2,1,0,0,0,0,0,0,128\n");
    check(jfg::DeterministicInputReplay::load(old.path(),replay)==jfg::InputReplayError::invalid_record,"v4 does not accept action semantics");
    const TemporaryReplay compatible("jfg-phase8-input-v5\n1,2,1,0,0,0,5,80,0,0\n");
    check(jfg::DeterministicInputReplay::load(compatible.path(),replay)==jfg::InputReplayError::none && replay.sample_at(1).modern_flags==0,"v5 retains legacy mapping semantics when disabled");
}

void test_native_menu_recording() {

  {
    const TemporaryReplay valid("jfg-phase8-input-v8\n1,2,1,8000,0,0,0,0,0,128,8,8\n");
    jfg::DeterministicInputReplay replay;
    check(jfg::DeterministicInputReplay::load(valid.path(),replay)==jfg::InputReplayError::none && replay.sample_at(1).menu_kind==8,"v8 Options direct selection survives recording");
    for(const auto *row:{"1,2,1,0,0,0,0,0,0,128,8,9","1,2,1,0,0,0,0,0,0,128,6,1","1,2,1,0,0,0,0,0,0,128,7,1","1,2,1,0,0,0,0,0,0,128,9,1"}) {
      const TemporaryReplay bad(std::string("jfg-phase8-input-v8\n")+row+"\n");
      check(jfg::DeterministicInputReplay::load(bad.path(),replay)==jfg::InputReplayError::invalid_record,"nonselection pages and invalid Options targets reject");
    }
    const TemporaryReplay old("jfg-phase8-input-v7\n1,2,1,8000,0,0,0,0,0,128,8,1\n");
    check(jfg::DeterministicInputReplay::load(old.path(),replay)==jfg::InputReplayError::invalid_record,"v7 does not reinterpret Options events");
  }
  {
    const TemporaryReplay validPause("jfg-phase8-input-v7\n100,101,1,0,0,0,0,0,0,128,5,7\n101,102,1,2000,0,0,0,0,0,128,5,9\n");
    jfg::DeterministicInputReplay replay;
    check(jfg::DeterministicInputReplay::load(validPause.path(),replay)==jfg::InputReplayError::none,"v7 pause events load");
    check(replay.sample_at(100).menu_target==7&&replay.sample_at(101).buttons==0x2000,"pause hover and native Save survive recording");
    const TemporaryReplay invalidPause("jfg-phase8-input-v7\n100,101,1,2000,0,0,0,0,0,128,5,10\n");
    check(jfg::DeterministicInputReplay::load(invalidPause.path(),replay)==jfg::InputReplayError::invalid_record,"invalid pause target rejected");
  }
    const TemporaryReplay valid("jfg-phase8-input-v6\n100,101,1,0,0,0,0,0,0,128,3,5\n101,102,1,8000,0,0,0,0,0,128,3,5\n");
    jfg::DeterministicInputReplay replay;
    check(jfg::DeterministicInputReplay::load(valid.path(),replay)==jfg::InputReplayError::none,"v6 direct menu selection loads");
    const auto hover=replay.sample_at(100);
    check(hover.menu_kind==3&&hover.menu_target==5&&!hover.buttons&&!hover.stick_x&&!hover.stick_y&&!hover.neutral(),"hover carries slot five without controller movement");
    check(replay.sample_at(101).buttons==0x8000&&replay.sample_at(101).menu_target==5,"click records selection and activation together");
    check(replay.sample_at(102).neutral(),"semantic event releases at EOF");
    const auto before=replay.events();
    for(const auto *row:{"1,2,1,0,0,0,0,0,0,0,3,7","1,2,1,0,0,0,0,0,0,0,2,4",
        "1,2,1,0,0,0,0,0,0,0,0,1","1,2,1,0,0,0,0,0,0,0,3,0",
        "1,2,1,0,0,0,0,0,0,0,1,1","1,2,1,0,0,0,0,0,0,0,4,2",
        "1,2,0,0,0,0,0,0,0,0,3,1","1,2,1,0,0,0,0,0,0,0,3"}) {
        const TemporaryReplay bad(std::string("jfg-phase8-input-v6\n")+row+"\n");
        check(jfg::DeterministicInputReplay::load(bad.path(),replay)==jfg::InputReplayError::invalid_record&&replay.events()==before,"invalid menu semantics reject atomically");
    }
}

void test_precise_stick_recording() {
    const TemporaryReplay valid("jfg-phase8-input-v4\n100,120,1,2010,80,-40,5,6144,-18432\n120,121,1,0,0,0,3,32000,0\n");
    jfg::DeterministicInputReplay replay;
    check(jfg::DeterministicInputReplay::load(valid.path(),replay)==jfg::InputReplayError::none,"v4 precise stick and mouse load");
    const auto sample=replay.sample_at(110);
    check(sample.pc_mode==5&&sample.look_x==6144&&sample.look_y==-18432&&sample.stick_x==80,"v4 preserves modern rate independently from movement");
    check(replay.sample_at(121).neutral(),"v4 look releases at EOF");
    const auto before=replay.events();
    for(const auto* row:{"1,2,1,0,0,0,5,18433,0","1,2,1,0,0,0,5,0,-18433",
        "1,2,0,0,0,0,5,1,0","1,2,1,0,0,0,6,0,0","1,2,1,0,0,0,4,128,0"}) {
      const TemporaryReplay bad(std::string("jfg-phase8-input-v4\n")+row+"\n");
      check(jfg::DeterministicInputReplay::load(bad.path(),replay)==jfg::InputReplayError::invalid_record&&replay.events()==before,"invalid v4 is rejected atomically");
    }
    const TemporaryReplay old("jfg-phase8-input-v3\n1,2,1,0,0,0,5,1,0\n");
    check(jfg::DeterministicInputReplay::load(old.path(),replay)==jfg::InputReplayError::invalid_record,"v3 never reinterprets precise rate mode");
    const TemporaryReplay compatible("jfg-phase8-input-v4\n1,2,1,0,0,0,4,80,0\n");
    check(jfg::DeterministicInputReplay::load(compatible.path(),replay)==jfg::InputReplayError::none&&replay.sample_at(1).look_x==80,"legacy mode keeps units in a v4 recording");
}

void test_repeated_poll_records() {
    const TemporaryReplay fixture(
        "jfg-phase8-input-v3\n"
        "21,22,1,8000,0,0,3,12,0\n"
        "21,22,1,0000,0,0,3,-7,0\n"
        "22,23,1,0010,0,0,0,0,0\n");
    jfg::DeterministicInputReplay replay;
    check(jfg::DeterministicInputReplay::load(fixture.path(), replay) ==
        jfg::InputReplayError::invalid_order, "retrace replay still rejects ambiguous repeated intervals");
    check(jfg::DeterministicInputReplay::load(fixture.path(), replay,
        jfg::InputReplayOrder::recorded_poll) == jfg::InputReplayError::none,
        "original OS may poll twice in one retrace");
    check(replay.events().size() == 3 && replay.sample_by_poll(0).buttons == 0x8000 &&
        replay.sample_by_poll(0).look_x == 12 && replay.sample_by_poll(1).look_x == -7 &&
        replay.sample_by_poll(2).buttons == 0x0010 && replay.sample_by_poll(3).neutral(),
        "recorded poll order preserves independent transitions and EOF");
    const auto before = replay.events();
    for (const auto* rows : {
        "21,23,1,0,0,0\n22,23,1,0,0,0\n",
        "21,22,1,0,0,0\n20,21,1,0,0,0\n",
        "21,23,1,0,0,0\n21,23,1,0,0,0\n"}) {
        const TemporaryReplay invalid(std::string("jfg-phase8-input-v2\n")+rows);
        check(jfg::DeterministicInputReplay::load(invalid.path(), replay,
            jfg::InputReplayOrder::recorded_poll) == jfg::InputReplayError::invalid_order &&
            replay.events() == before, "poll mode rejects reordered or overlapping intervals atomically");
    }
}

void test_rejects_invalid_records() {
    for (const std::string_view contents : {
             "wrong-header\n1800,1810,1000,0,0\n",
             "jfg-phase8-input-v1\n1800,1800,1000,0,0\n",
             "jfg-phase8-input-v1\n1800,1810,10000,0,0\n",
             "jfg-phase8-input-v1\n1800,1810,1000,-129,0\n",
             "jfg-phase8-input-v1\n1800,1810,1000,0,0\n1809,1820,0,0,0\n",
             "jfg-phase8-input-v2\n1800,1810,2,0000,0,0\n",
             "jfg-phase8-input-v2\n1800,1810,0,8000,0,0\n",
         }) {
        const TemporaryReplay fixture(contents);
        jfg::DeterministicInputReplay replay;
        check(jfg::DeterministicInputReplay::load(fixture.path(), replay) !=
                  jfg::InputReplayError::none,
              "invalid replay should fail closed");
    }
}

}  // namespace

int main() {
    test_valid_replay();
    test_independent_look();
    test_precise_stick_recording();
    test_modern_action_recording();
    test_native_menu_recording();
    test_long_manual_recording();
    test_connection_transitions();
    test_poll_replay_eof_is_neutral();
    test_repeated_poll_records();
    test_rejects_invalid_records();
    return failures == 0 ? 0 : 1;
}
