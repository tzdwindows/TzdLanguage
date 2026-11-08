# PowerShell script to sync wiki/ directory to GitHub Wiki git repository
$wikiGitUrl = "https://github.com/tzdwindows/TzdLanguage.wiki.git"
$tempDir = Join-Path $PSScriptRoot ".temp_wiki"

Write-Host "[1/4] Checking GitHub Wiki repository..." -ForegroundColor Cyan
$remoteCheck = git ls-remote $wikiGitUrl 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] GitHub Wiki repository is not initialized yet!" -ForegroundColor Red
    Write-Host "Please go to: https://github.com/tzdwindows/TzdLanguage/wiki" -ForegroundColor Yellow
    Write-Host "Click 'Create the first page' and save it once. Then run this script again." -ForegroundColor Yellow
    exit 1
}

Write-Host "[2/4] Cloning Wiki repository..." -ForegroundColor Cyan
if (Test-Path $tempDir) {
    Remove-Item $tempDir -Recurse -Force
}
git clone $wikiGitUrl $tempDir

Write-Host "[3/4] Copying Wiki markdown documents..." -ForegroundColor Cyan
Copy-Item (Join-Path $PSScriptRoot "wiki\*") $tempDir -Recurse -Force

Write-Host "[4/4] Committing and pushing to GitHub Wiki..." -ForegroundColor Cyan
Push-Location $tempDir
git add .
git commit -m "docs(wiki): sync technical wiki documentation from main repository"
git push origin master
Pop-Location

Remove-Item $tempDir -Recurse -Force
Write-Host "[SUCCESS] Technical Wiki successfully deployed to https://github.com/tzdwindows/TzdLanguage/wiki" -ForegroundColor Green
