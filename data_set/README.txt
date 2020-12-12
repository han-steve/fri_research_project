#Welcome to our dataset generation script! 
To run this, you probably need a Mac (haven't tested on any other device).
Oh wait does this mean we can't run this on Rudi's linux machine? If we want to make it work we would need to install mujoco separately on his machine. Or we can generate the data and copy the data over via ssh (this could be time-consuming). 

##How to: 
run make.
run  ./generate_data pool.xml 3 10 1 (that last number is the amount of data points to generate)

Now, there would be some files labeled 0_0,1,2... .out  and 1_0,1,2... .out. The beginning 0 and 1 indicate which goal the balls are going into.
If you want to visualize the raw videos, install ffmeg and run
ffmpeg -f rawvideo -pixel_format rgb24 -video_size 800x800 -framerate 10 -i 0_0.out -vf "vflip" 0_0.mp4

Have fun generating data!
