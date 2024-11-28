
./install.csh
cd llwxjson
zip ../llwxjson.zip *
cd ..

scp llwxjson.zip ec2-44-212-16-12.compute-1.amazonaws.com:/tmp/
 