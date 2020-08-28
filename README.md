## PortRecord (port7.c)
Uses port audio and libsndfile to enable real-time recording in a basic command-line program
Must have libsndfile and PortAudio installed before use
Compile: cc -o portrecord Port_Record.c -lsndfile -lportaudio
Run: ./portrecord outfilename.wav (must supply with output file name for recording)
