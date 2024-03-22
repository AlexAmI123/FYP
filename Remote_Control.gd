# Remote_Control.gd
# This script extends Node2D to create a remote control system using TCP communication.
# It's designed to send commands to a server based on user input.

extends Node2D

# TCP connection parameters including the server IP and the port to connect for commands
var server_ip: String = "127.0.0.1"  # The IP address of the server (localhost in this case)
var command_port: int = 8080  # The port on which to send command data
var connection: StreamPeerTCP  # Variable to hold the TCP connection object

# Called when the node is added to the scene tree, initializes the connection
func _ready():
	connection = StreamPeerTCP.new()  # Creates a new StreamPeerTCP instance
	connect_to_server()  # Calls the function to connect to the server

# Connect to the TCP server using the specified IP address and port
func connect_to_server():
	var error = connection.connect_to_host(server_ip, command_port)  # Attempts to connect to the server
	if error != OK:
		print("Failed to connect to server at %s:%d" % [server_ip, command_port])  # Error handling if connection fails
	else:
		print("Successfully connected to server at %s:%d" % [server_ip, command_port])  # Success message on connection

# Send a command to the TCP server. This function is called with a string command
func send_command(command: String):
	if connection.get_status() == StreamPeerTCP.STATUS_CONNECTED:  # Checks if the connection is still active
		var data = command.to_utf8_buffer()  # Converts the command string to UTF-8 byte buffer
		connection.put_data(data)  # Sends the data through the connection
	else:
		print("Connection lost. Attempting to reconnect...")  # If connection is lost, tries to reconnect
		connect_to_server()  # Reconnection attempt

# Input handler for sending commands. This listens for keyboard inputs
func _input(event):
	if event is InputEventKey and event.pressed and !event.echo:  # Filters for non-repeated key press events
		if event.scancode == KEY_KP_8:  # If the Numpad 8 key is pressed
			send_command("walk_forward")  # Sends the "walk_forward" command to the server
