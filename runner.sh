while true; do
	make && cp release/quake2 G:/Quake_2_Runner \
		 && cp release/baseq2/game.dll G:/Quake_2_Running/baseq2 \
		 && cd G:/Quake_2_Running && ./quake2 +map base1 \
		 && cd G:/Quake_2
done