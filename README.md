# RocketLeagueCUE
WIP Rocket League Boost Visualizer.

## Notes

- A lot of this code is shit. This is my first bout with C++ and windows programming in general. I want my Linux back.

- This code was built on top of the CDK SDK "progress" example, which is why folders are named as such. It's possible this code isn't compilable out of box, in which case, toss it in the CUE SDK examples folder, and it should work.

- I only have a Strafe RGB to test on. All other keyboards will have unknown behaviour.

## Other notes
The code works with my K68 keyboard, but I only have red leds ¯\_(ツ)_/¯.

This fork is simply my attempt to understand this program and how to make it work. I thought that 5 years might have brought breaking changes in the game that would make the old code unusable, but after changing 90% of the lines, it seems like the only noticeably different thing I changed was the pointer offsets... 

### Set up the build
This section might be useless for most people, but it should help c++ noobs like me.
- Download the lastest CUE SDK release: https://github.com/CorsairOfficial/cue-sdk
- Go into the `examples` folder in the CUE SDK and clone this repo there
- Open Visual Studio, and open the project by using the .sln file
- Build and run the project with Ctrl+F5

### How to find the offsets
- You'll need Cheat Engine in order to find your multi level pointer offsets. You'll also need a basic understanding of how to use Cheat Engine (you can try the Cheat Engine tutorial and do the first 6 steps) This video explains very well how to do an advanced pointer scan to find the static address that we're looking for (You should watch the first part, but the important bit starts at 9 minutes) https://www.youtube.com/watch?v=nQ2F2iW80Fk. In my case, I found around 30000 pointers for the boost value, so I just took a random one and it seems like it works even if I restart the game.
- Once you have the base address and the offsets, head over to progress/main.cpp at the beginning of the file (around line 30), change the values by those that you've found.
