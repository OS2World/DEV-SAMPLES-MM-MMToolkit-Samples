:userdoc.
:h1 res=001.Help for String Test
:p.
The String Test sample program allows the user to control any media device
that is
installed on his system.  The utility
accepts Media Control Interface string commands from the user, and
passes them on to the appropriate multimedia component for execution.
:p.
Type the desired string command in the :hp2.String command:ehp2.
entry field.
The valid format for a string command is <COMMAND> <DEVICE> <PARAMETERS>.
For example, the following three string commands will play a waveform
device that resides in the current directory:
:ul compact.
:li."open filename.wav alias xxx"
:li."play xxx notify"
:li."close xxx"
:eul.
:p.
Select the :hp2.Send:ehp2. push button to send a string command to
the appropriate multimedia component for execution.  In the example above,
pressing the :hp2.Send:ehp2. push button after each string command
would send each command
to the wave audio device.
:p.
For more information about string commands, please refer to the MMPM/2
Programmer's
Reference.
:p.
Selecting the :hp2.Send:ehp2. push button will also add the string command
to the list box below the :hp2.String command:ehp2.
entry field.  The user may scroll through this list box and select from it
in order to complete the :hp2.String command:ehp2. entry field.
:p.
Select the :hp2.Display messages:ehp2. push button to display a pop-up
window containing notification messages that the String Test
utility is receiving from media devices.
:p.
Select the :hp2.Cancel:ehp2. push button to end the application.
:p.
Select the :hp2.Help:ehp2. push button to view this help panel.
:euserdoc.
