#!/bin/csh -f

set app=llwxjson
xcodebuild -list -project $app.xcodeproj

# rm -rf DerivedData/
xcodebuild -configuration Release -alltargets clean
xcodebuild -scheme $app -configuration Release build
xcodebuild -scheme $app -configuration Release build

find ./DerivedData -type f -name $app -perm +111 -ls
set src=./DerivedData/$app/Build/Products/Release/$app
set src=./DerivedData/Build/Products/Release/$app

echo "File $src"
ls -al $src
cp $src ~/opt/bin/