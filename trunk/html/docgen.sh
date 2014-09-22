java \
-Djsdoc.dir=./jsdoc-toolkit \
-jar ./jsdoc-toolkit/jsrun.jar \
./jsdoc-toolkit/app/run.js \
-t=./jsdoc-toolkit/templates/jsdoc \
-r=4 \
./sincity.js \
-d=docgen \
-D="title:SinCity Library" \
-D"index:files"

#cp docgen.index.html docgen/index.html

sed -i 's/="assets/="..\/assets/g' docgen/index.html
sed -i 's/="docgen\//="/g' docgen/index.html
sed -i 's/="images/="..\/images/g' docgen/index.html

#sed -i 's/Namespace /AnonymousClass /g' docgen/symbols/SCCall.html
#sed -i 's/Namespace /AnonymousClass /g' docgen/symbols/SCMessage.html
