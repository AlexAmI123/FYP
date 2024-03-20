extends Node2D

# TCP connection parameters
var server_ip: String = "127.0.0.1"
var command_port: int = 8080
var connection: StreamPeerTCP

# Called when the node enters the scene tree for the first time.
func _ready():
	connection = StreamPeerTCP.new()
	connect_to_server()

# Connect to the TCP server
func connect_to_server():
	var error = connection.connect_to_host(server_ip, command_port)
	if error != OK:
		print("Failed to connect to server at %s:%d" % [server_ip, command_port])
	else:
		print("Successfully connected to server at %s:%d" % [server_ip, command_port])

# Send a command to the TCP server
func send_command(command: String):
	if connection.get_status() == StreamPeerTCP.STATUS_CONNECTED:
		var data = command.to_utf8()
		connection.put_data(data)
	else:
		print("Connection lost. Attempting to reconnect...")
		connect_to_server()

# Input handler for sending commands based on keyboard input
func _input(event):
	if event is InputEventKey and event.pressed and !event.echo:
		if event.scancode == KEY_KP_8:  # Numpad 8
			send_command("walk_forward")
