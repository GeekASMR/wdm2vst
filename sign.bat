@echo off
set BIN="C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64"
set INF2CAT="C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\Inf2Cat.exe"

echo Generaing Catalog...
%INF2CAT% /driver:"d:\Autigravity\wdm2vst" /os:10_X64

echo Making Test Cert...
%BIN%\MakeCert.exe -r -pe -ss PrivateCertStore -n "CN=WDM2VST_TestCert" -eku 1.3.6.1.5.5.7.3.3 wdm2vst_test.cer

echo Installing Cert to Trusted Root and Publisher...
%BIN%\certmgr.exe -add wdm2vst_test.cer -s -r localMachine root
%BIN%\certmgr.exe -add wdm2vst_test.cer -s -r localMachine trustedpublisher

echo Signing the Catalog...
%BIN%\signtool.exe sign /v /s PrivateCertStore /n WDM2VST_TestCert /fd sha256 /tr http://timestamp.digicert.com /td sha256 wdm2vst_4ch.cat

echo Checking the signature...
%BIN%\signtool.exe verify /v /pa wdm2vst_4ch.cat
echo Done!
