# Commits the current community changes in this checkout and pushes them to your fork.
$ErrorActionPreference = 'Continue'
$root = Split-Path $PSScriptRoot
$branch = 'community/on-main'
$fork = 'https://github.com/lukedeardoff/JetForceGemini-Recomp.git'
$msg = Join-Path $PSScriptRoot '.commit-message.txt'
git -C $root add -- src include scripts patches community
git -C $root -c user.name=lukedeardoff -c user.email=lukedeardoff@users.noreply.github.com commit -F $msg
Write-Host "Pushing to your fork (sign in as lukedeardoff if asked)..." -ForegroundColor Cyan
git -C $root push $fork "${branch}:${branch}"
if ($LASTEXITCODE -eq 0) { Write-Host ""; Write-Host "DONE - saved to GitHub." -ForegroundColor Green }
else { Write-Host ""; Write-Host "Push failed - tell Claude." -ForegroundColor Red }
