#!/bin/sh

# uploader.sh

if [ "$REQUEST_METHOD" = "GET" ]; then

	cat <<END
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 2.0//EN">
<html>
<head>
  <title>Sample File Upload Form</title>
</head>
<body>
  <h1>Sample File Upload Form</h1>
  <h2>Please fill in the file-upload form below</h2>
  <hr>
  <form method='post' enctype='multipart/form-data' action='uploader.sh'>
    File to upload: <input type="file" name="upfile"><br>
    <br>
    Notes about the file: <input type="text" name="note"><br>
    <br>
    <input type="submit" value="Press"> to upload the file!
  </form>
  <hr>
</body>
</html>
END

	exit 0

elif [ "$REQUEST_METHOD" = "POST" ]; then

	DIR=uploads

	if [ ! -d ../$DIR ]; then
		mkdir -p ../$DIR
	fi

	mv $1	../$DIR

	FILE=/$DIR/`basename $1`

	cat <<END
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
  <title></title>
</head>
<body>
  <table align="center" class="table">
    <tr>
      <td class="table_header" colspan="2"><b>Your file have been uploaded!</b></td>
    </tr>
    <tr>
      <td class="table_body"><br>
      <b>File #1:</b> <a href="$FILE" target="_blank">$FILE</a><br>
      <br>
      <br>
      <a href="/cgi-bin/uploader.sh">Go Back</a><br></td>
    </tr>
  </table>
</body>
</html>
END

	exit 0
fi

# printenv -- just prints its environment

echo -e 'Content-Type: text/html; charset=utf8\r\n\r'
echo '<pre>'
env
echo '</pre>'

exit 1
