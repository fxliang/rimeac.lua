setlocal

rem exact librime
set rime_version=1.11.2
set rime_hash=5b09f35

set download_archive=rime-%rime_hash%-Windows-msvc-x86.7z
set download_archive_x64=rime-%rime_hash%-Windows-msvc-x64.7z

if not exist %download_archive% (
  curl -LO https://github.com/rime/librime/releases/download/%rime_version%/%download_archive%
)
if not exist %download_archive_x64% (
  curl -LO https://github.com/rime/librime/releases/download/%rime_version%/%download_archive_x64%
)

7z x %download_archive% * -olibrime\ -y
7z x %download_archive_x64% * -olibrime_x64\ -y

copy /Y librime\dist\include\rime_*.h include\
copy /Y librime\dist\lib\rime.lib lib\
copy /Y librime\dist\lib\rime.dll dist\
copy /Y librime\dist\lib\rime.dll lib\

copy /Y librime_x64\dist\lib\rime.lib lib64\
copy /Y librime_x64\dist\lib\rime.dll dist64\
copy /Y librime_x64\dist\lib\rime.dll lib64\

rmdir /s /q librime librime_x64
