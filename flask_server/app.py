from flask import Flask, escape, request, g, send_file
import sqlite3
import os
import io
import zipfile
import math
import subprocess

db_path = '../test.db'
get_tracks_query = 'SELECT rowid,* FROM tracks;'
get_path_query = 'SELECT path,title from tracks WHERE rowid='
get_album_query = 'SELECT path,album from tracks WHERE album = ( SELECT album FROM tracks WHERE track=1 AND rowid='
static_dir = 'static/'
media_dir = 'media/'

app = Flask(__name__)

def get_db():
	db = g.get('_database',None)
	if db is None:
		db = g._database = sqlite3.connect(db_path)
	return db

@app.route('/')
def get_tracks():
	name = request.args.get("name","World")
	cursor = get_db().cursor()
	cursor.execute(get_tracks_query)
	tracks = cursor.fetchall()
	
	return {
		"tracks": [{
			"id": t[0],
			"title": t[1],
			"artist": t[2],
			"album": t[3],
			"year": t[4],
			"track": t[5],
			"genre": t[6],
		} for t in tracks]
	}

@app.route('/stream/album/<int:id>')
def stream_album(id):
	return "..."

@app.route('/stream/track/<int:id>')
def stream_track(id):
	cursor = get_db().cursor()
	cursor.execute(get_path_query+str(id))
	results = cursor.fetchone()

	path = results[0]
	title = results[1]

	print(path)
	file_name = "media/{}.m3u8".format(str(id))
	if (not os.path.exists(file_name)):
		subprocess.call("./segment.sh \"{}\" \"{}\" {}".format(
			path,
			media_dir,
			str(id)
		),shell=True)	
		'''
		with open(file_name,"wb") as file_data:
			file_data.write("#EXTM3U\n".encode('utf-8'))
			file_data.write("#EXT-X-VERSION:6\n".encode('utf-8'))
			file_data.write("#EXT-X-TARGETDURATION:10\n".encode('utf-8'))
			file_data.write("#EXT-X-PLAYLIST-TYPE:VOD\n".encode('utf-8'))
			file_data.write("#EXT-X-ALLOW-CACHE:YES\n".encode('utf-8'))
			for i in range(0,num_chunks+1):
				file_data.write("#EXTINF:10.000\n".encode('utf-8'))
				file_data.write("/{}{}_{}.ts\n".format(
					media_dir,str(id),str(i)
				).encode('utf-8'))
			file_data.write("#EXT-X-ENDLIST\n".encode('utf-8'))
			file_data.seek(0)
		'''
	
	return send_file(
		file_name,
		mimetype='text/plain',
		as_attachment=True,
		attachment_filename=title+".m3u8"
	)
	
@app.route('/download/album/<int:id>')
def download_album(id):
	cursor = get_db().cursor()
	cursor.execute(get_album_query+str(id)+")")
	results = cursor.fetchall()
	path = results[0][0]
	album = results[0][1]
	file_data = io.BytesIO()
	f = zipfile.ZipFile(file_data,mode="w")
	for track in results:
		path = track[0]
		f.write(path,os.path.basename(path))
	f.close()
	file_data.seek(0)
	return send_file(
		file_data,
		mimetype='application/zip',
		as_attachment=True,
		attachment_filename=album+".zip"
	)

@app.route('/download/track/<int:id>')
def download_track(id):
	cursor = get_db().cursor()
	cursor.execute(get_path_query+str(id))
	results = cursor.fetchone()
	path = results[0]
	title = results[1]
	file_data = io.BytesIO()
	f = zipfile.ZipFile(file_data,mode="w")
	f.write(path,os.path.basename(path))
	f.close()
	file_data.seek(0)
	return send_file(
		file_data,
		mimetype='application/zip',
		as_attachment=True,
		attachment_filename=title+".zip"
	)

@app.route('/stream/track/<string:file>')
def download_chunk(file):
	return send_file(media_dir+file)

@app.teardown_appcontext
def close_connection(exception):
	db = g.get('_databse',None)
	if db is not None:
		db.close()
