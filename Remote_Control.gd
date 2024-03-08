extends Node2D

func _input(event):
	if event is InputEventKey and event.pressed and !event.echo:
		if event.scancode == KEY_KP_8:  # Check if +Numpad 8 is pressed
			print("Sent walk_forward")
			$TCPSender.send_command("walk_forward")
