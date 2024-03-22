# CameraFeedBottom.gd
# This script extends TextureRect to display a camera feed from a network source.

extends TextureRect

# Default port and server IP address for the camera feed
var port: int = 8082 
var server_ip: String = "127.0.0.1"  # Defaulting to localhost for testing or local network use
var connection: StreamPeerTCP  # Declares a variable to hold the TCP connection

func _ready():
	# This method is called when the node is added to the scene and is ready.
	connection = StreamPeerTCP.new()  # Initializes the StreamPeerTCP object
	var error = connection.connect_to_host(server_ip, port)  # Tries to connect to the specified host and port
	if error != OK:
		print("Failed to connect to camera feed on port ", port)  # Prints an error message if the connection fails
	else:
		print("Connected to camera feed on port ", port)  # Prints a success message if the connection is established
		set_process(true)  # Enables the _process function to be called every frame

func _process(delta):
	# This method is called every frame and is used to update the camera feed.
	# Checks if the connection is active and if there are bytes available to read
	if connection.get_status() == StreamPeerTCP.STATUS_CONNECTED and connection.get_available_bytes() > 0:
		var jpeg_data = connection.get_var()  # Reads JPEG data from the connection
		var image = Image.new()  # Creates a new Image object
		var error = image.load_jpg_from_buffer(jpeg_data)  # Attempts to load the JPEG data into the Image object
		if error == OK:
			var texture = ImageTexture.new()  # Creates a new ImageTexture object
			texture.create_from_image(image)  # Creates a texture from the loaded image
			self.texture = texture  # Updates the TextureRect's texture to display the new image
