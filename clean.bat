@echo off
echo Limpando projeto React Native...

echo Parando processos do Metro...
taskkill /f /im node.exe 2>nul

echo Removendo node_modules...
if exist node_modules rmdir /s /q node_modules

echo Limpando cache do pnpm...
pnpm store prune

echo Reinstalando dependências...
pnpm install

echo Limpando build do Android...
cd android
if exist build rmdir /s /q build
if exist app\build rmdir /s /q app\build
gradlew clean
cd ..

echo Limpeza concluída!
echo Execute: npx react-native start --reset-cache
pause