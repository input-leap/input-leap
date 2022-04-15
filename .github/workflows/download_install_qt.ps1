$ErrorActionPreference = "Stop"

$qli_install_version = '2019.05.26.1'
$qt_version = '5.15.2'

New-Item -Force -ItemType Directory -Path ".\deps\"

Write-Output 'Downloading QLI Installer'
Invoke-WebRequest "https://github.com/nelsonjchen/qli-installer/archive/v$qli_install_version.zip" -OutFile '.\deps\qli-installer.zip' ;
Write-Output 'Downloaded QLI Installer'

Write-Output 'Extracting QLI Installer'
Expand-Archive deps\qli-installer.zip deps\
Move-Item .\deps\qli-installer-$qli_install_version\ .\deps\qli-installer
Write-Output 'Extracted QLI Installer'

Write-Output 'Installing QLI Installer Dependencies'
pip install -r .\deps\qli-installer\requirements.txt
Write-Output 'Installed QLI Installer Dependencies'

Write-Output 'Starting QT Installer'
$Env:QLI_OUT_DIR = ".\deps\Qt\Qt$qt_version"
$Env:QLI_BASE_URL = "https://download.qt.io/online/qtsdkrepository/"
python .\deps\qli-installer\qli-installer.py $qt_version windows desktop win64_msvc2019_64
Write-Output 'Installed QT Installer'
