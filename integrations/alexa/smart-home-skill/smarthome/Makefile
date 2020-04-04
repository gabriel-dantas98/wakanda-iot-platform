zip:
	zip -r build.zip ./  	

package-build:
	pip install -r requirements.txt -t ./

virtualenv:
	source venv/bin/activate

virtualenv-create:
	virtualenv venv --python=python3

upload-function:
	aws lambda update-function-code \
    --function-name smart-home-skill \
    --zip-file fileb://build.zip
