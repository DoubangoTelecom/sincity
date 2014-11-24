#!/bin/bash

API_VERSION=1.0.0
API_FOLDER_NAME=release
API_FILE_NAME=sincity.js
API_FILE_PATH=$API_FOLDER_NAME/$API_FILE_NAME

# src dst
CompressFile()
{	
	echo Compressing ... $1 to $2
	if [ ${1: -3} == ".js" ]
	then
		# java -jar google-closure-compiler.jar --js $1 --js_output_file $2 --charset utf-8
		java -jar yuicompressor-2.4.7.jar $1 -o $2 --charset utf-8
	else
		java -jar yuicompressor-2.4.7.jar $1 -o $2 --charset utf-8
	fi
}

# src dst
AppendFile()
{
	echo Appending... $1 to $2
	cat $1 >> $2
}

#dst
AppendScripts()
{
	echo "var __b_release_mode = true;" > $1
	
	AppendFile ./sincity.js $1
    
	# at this step 'tsk_utils_log_info' is defined
	echo "console.info('SINCITY API version = $API_VERSION');" >> $1
}

# src dst
DeployFile()
{
	if [ ${1: -3} == ".js" ] || [ ${1: -4} == ".css" ]
	then
		CompressFile $1 $2
	else
		echo copying to... $2
		cp -f $1 $2
	fi
}

# folder
DeployFolder()
{
	for src_file in $(find $1 -name '*.js' -o -name '*.htm' -o -name '*.html' -o -name '*.css' -o -name '*.wav' -o -name '*.png' -o -name '*.bmp')
	do 
		name=`basename $src_file`
		src_dir=`dirname "$src_file"`
		base=${src_file%/*}
		
		dest_dir=$API_FOLDER_NAME/${src_dir: 0}
		dest_file=$dest_dir/$name
		mkdir -p $dest_dir
		
		DeployFile $src_file $dest_file
	done
}

# deploy assets
DeployFolder assets

# deploy images
DeployFolder images

# deploy sounds
DeployFolder sounds

# deploy html files
for file in test_client.htm test_server.htm index.html
do
	DeployFile $file $API_FOLDER_NAME/$file
done

# append JS scripts
AppendScripts $API_FILE_PATH.tmp.js
# compress JS scripts
CompressFile $API_FILE_PATH.tmp.js $API_FILE_PATH
rm -rf $API_FILE_PATH.tmp.js

# adapter.js
DeployFile adapter.js $API_FOLDER_NAME/adapter.js

# generate and deploy documentation
./docgen.sh
DeployFolder docgen


