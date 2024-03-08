extends TextureRect

var port: int = 8082 
var server_ip: String = "127.0.0.1"  # Defaulting to localhost
var connection: StreamPeerTCP

func _ready():
	connection = StreamPeerTCP.new()
	var error = connection.connect_to_host(server_ip, port)
	if error != OK:
		print("Failed to connect to camera feed on port ", port)
	else:
		print("Connected to camera feed on port ", port)
		set_process(true)

func _process(delta):
	# Correctly check if the connection is active
	if connection.get_status() == StreamPeerTCP.STATUS_CONNECTED and connection.get_available_bytes() > 0:
		var jpeg_data = connection.get_var()
		var image = Image.new()
		var error = image.load_jpg_from_buffer(jpeg_data)
		if error == OK:
			var texture = ImageTexture.new()
			texture.create_from_image(image)
			self.texture = texture
