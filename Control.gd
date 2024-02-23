extends Control

var server_bottom = TCPServer.new()
var server_top = TCPServer.new()
var clients = []

func _ready():
	start_server(server_bottom, 8081)
	start_server(server_top, 8082)
	set_process(true)

func start_server(server, port):
	server.listen(port)
	print("Server listening on port ", port)

func _process(delta):
	check_for_connections(server_bottom, "bottom")
	check_for_connections(server_top, "top")
	for client in clients:
		if client.get_available_bytes() > 0:
			var data = client.get_var()
			update_camera_feed(data, client.userdata)

func check_for_connections(server, position):
	if server.is_connection_available():
		var client = server.take_connection()
		client.set_blocking_mode(false)
		clients.append(client)
		client.userdata = position # Use userdata to store the position

func update_camera_feed(data, position):
	var image = Image.new()
	var error = image.load_jpg_from_buffer(data)
	if error == OK:
		var texture = ImageTexture.new()
		texture.create_from_image(image)
		if position == "bottom":
			$CameraFeedBottom.texture = texture
		elif position == "top":
			$CameraFeedTop.texture = texture
