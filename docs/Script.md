# **This is the general script of the machine interaction**

# Intro:

While the machine is idle and there is no one nearby. Have all the lights off (with the exception of the big red button dimly lit) and occasionally play some voicelines: “Hello?”, “ta ta ta”, “tsk hmm hm hmm”, \*whistle\*

If the machine detects someone nearby, play voice lines to call out to them. Maybe faintly turn on some lights or increase the brightness of the glowing red button. The glowing red button should pulse light. Voicelines to play: “Hey you there\!”, “Person… thing… detected movement”, “Help me”

If the person approaches the machine, start the next section titled: Press the Button

# Press the Button:

The machine still appears mostly powered off. The glowing red button should be pulsing at full brightness. The machine calls out the the person with the following voice lines:  
“Hey can you help power me on?”  
“Press the big red glowing button”  
“It may say something like *do not press*… you can ignore that”

The user may hesitate to press the button. During this time occasionally cycle through these voice lines: “Just click it”, “c’mon its right there”, “clickity click click”, “big red button… calling your name”

If at any time during the startup sequence the person walks away from the machine, play the following voicelines: “No wait don’t back away”

If the person presses the button, go to the next section titled: Startup Sequence

# Startup Sequence:

As soon as the person presses the big red glowing button, turn off the LED in the button. Start playing a loud alarm sound. Flash all the lights in the machine. Release a few marbles from the marble run (this is probably done with a solenoid). Power on the LED matrix and begin a 5 OR 10 minute countdown on the display. The odds should be random on which time is chosen. Stop flashing the lights and alarm after a few seconds. Turn on the lights in the machine but don't turn on the lights in the button. Power on the screen and play one of the following voice lines and associated video: “Oh hooray\! You actually did it?\!\!” or “Thank you so much\!” or “You really– thanks a million\!”

After this move to the next sequence: A Big Problem

# A Big Problem:

Play the following voice line and video: “Now I don’t mean to alarm you but the button may have started a self destruct sequence”

If the countdown was set to 10 minutes play the following voice line and video: “Uh let me try fixing it”. Then halve the value of the timer. Immediately jump to the next voiceline.

Play the following voice line and video: “AH\! Uhh try pulling that small lever on the left\!”

If the person hesitates, play the following idle voicelines: “lever\! On your left\!”, “You ever use a Bop It?”, “ahem\! Timer, lever\!”

As soon as the person starts pulling the lever, release the electromagnet holding it on causing the lever to fall off. Play the next sequence: AUGHHH

# AUGHHH:

Play the voiceline and video: “AUGHHH Put it back\! Put it back\! Put it back\!”   
Turn the electromagnet back on.

If the person hesitates in putting the lever back on cycle through the following voice lines and videos: “Ah why’d you do that?\!”, “put it back on”

Once the person reattaches the lever play the voice line and video: “whew, thanks” and move on to the next section: Dollar Eater

If the person does not reattach the lever after 30 or 40 seconds play the voice line and video: “Forget about the lever” then move on to the next section: Dollar eater

# Dollar Eater:

Play voice line and video: “Hey do you have a dollar?”. Then light up the buttons labeled *yes* and *no*.  
While idly waiting for a response play the lines and videos: “I mean you were the one who broke my lever”, “i feel like yout a little obligated”

If the person presses the yes/no buttons during an idle line, interrupt the voice line and continue on.

If the person presses no, play the voiceline and video: “No? WEAK”. Then skip to the next sequence: Armed Button

If the person presses yes play the voice line and video: “Oh we’re saved” OR “Thank goodness”  
Then light up the dollar eater (if possible) and play the voiceline and video: “ok can you put one crisp US dollar into the money slot”. Play an idle animation on the screen of the character looking around/twiddling their thumbs. Idle voiceline and video if person hesitates: “I also take fives… tens…hundreds” go back to idle animation after idle voiceline and video.

Once the person starts putting the bill in (detected with a switch), turn on the motor in the dollar eater to pull in the bill. Play machine processing noises on the speakers. Play a loading animation on the LEDs inside the machine (not the matrix). After a few seconds play a rejected noise, turn the lights in the machine red, and display REJECTED on the led matrix. Then return the lights and timer to normal and play voice line and video: “huh shame” then move onto the next sequence: Armed button

# Armed Button:

Play voiceline and video of machine character grabbing and flipping through a book: “hmmm ‘To reset the something something green button inside the machine’ ” Play an animation of the character looking around and then the voiceline: “There\! Oh ok I see what's going on here”

Light up one of the front machine buttons blue, play the voice line and video: “The blue button you have should move the hand”

When the person presses the blue button, play a little button noise, turn off the blue LED in the button, and start slowly moving the linear actuator. Play the voice line and video: “ah here we go”. After like 2 seconds stop the linear actuator abruptly. Play the voice line and video: “What happened? Did you do something? Try pressing the button again” Light up the blue button again

When the person presses the blue button, play the button noise again, turn off the blue LED in the button, and again start moving the linear actuator. Play the voiceline and video “Back on track smooth– AGAIN?”. After like only one second stop the linear actuator abruptly. Voice line and video: “you know what to do”. Light up the blue button again.

When the person presses the blue button, play a little button noise, turn off the blue LED in the button, and start slowly moving the linear actuator. This time, stop it after only half a second. Play voice line and video: “Ok it's definitely moving less now. I think you’re going to have to mash that button” Light the button up blue again.

Each time the user presses the button play the button noise, turn off the blue led only while the button is pressed down and turn it back on as soon as the button is released. For each button press, move the linear actuator forward a random small amount. After about 4 seconds of mashing, randomize the direction the actuator moves so it starts moving forward and backwards (😂). After another couple seconds turn off the blue led in the button for good and make the linear actuator move to fully press green the button. Play a little victory jingle sound effect once the green button is pressed. Release 2 or 3 marbles onto the marble run with the solenoid.

Play voice line and video: \*character picks up book again to read\* “to reset the gyroscope, marble run, and cryocooler press the green button inside the machine… Reading comprehension tsk tsk” Move on to the next sequence: The Punch

# The Punch

Play the voice line and video: “My gut is saying the big lever on the right is our ticket out of this mess”. Release whatever mechanism is preventing the lever from being pulled and the punching arm to come out. Play Idle rambling voice lines and video: “I mean if I were trying to make an emergency shut off I’d probably make it a big lever. Why didn’t we try that first? Right like one big ka-thunk and the timer stops. Poof just like that. No more countdown, flashing lights. One big pull of the lever” The person is expected to pull the lever while the machine is rambling on.

When the person pulls the lever, interrupt the rambling with the voiceline and animation: “HAH\! Did it get you? Really?” Light up the yes/no buttons for a second then play the voiceline and video: “Actually doesn't really matter”. Go to the next sequence: Sliders

# Sliders

Play the voice line and video: “ You know you haven't really been all that helpful. I’m gonna go try and fix the problem on my own, how about in the meantime you just play around with those sliders” \*Character walks off screen\*.

There are three linear sliders. Choose a position on each slider that is somewhat far from its current position. As soon as a slider is moved, fade in a droning “noooooo” sound. As a slider gets closer to its target position, crossfade the droning “noooo” into a droning “yeeessss”. The no and yes audio files will be made to loop cleanly.

Once all three sliders reach their yes position (plus or minus some margin of error) Play a ding noise and an audio line “Self Destruct Sequence Cancelled”. Display “STATUS GOOD” on the LED matrix.

Play voice line and video: \*Character walks back on screen\* “I think I figured out how to disab– oh… No it’s fine, I get it. You don’t need me. I had a whole plan of fun activities for us to do” \*holds up open binder with fun planner\* “and maybe I started the self-destruct sequence. But we were having a good time\!” Play idle sad animation on screen. Change LED matrix to say “STATUS :(”. Move to the next sequence: Reconciliation

# Reconciliation:

Light up the green start button.

When the person presses the start button, play robotic “start” sound, turn off the led in the button. Play voice line and video: “aw your right. Lets start over. What’s your favorite number?”

Wait until a number is pressed on the keypad. After a digit is pressed, give a grace period of one second for the user to type a second digit. Reset the grace period each time a digit is pressed. The user should be allowed to enter up to 16 digits. After waiting 2 seconds from the last number input OR reaching 16 digits play the next voice line and video: “No way\! My favorite number is also” \*persons number\*. In order to have the character say the person's number they just entered you will have to play clips one after the other. It will intentionally look and sound choppy. There will be recordings of saying numbers 1-20,20,30,40,50,60,70,80,90, hundred,thousand,million,billion,trillion,quadrillion. EXAMPLE: If the user enters 158,204 the voicelines would be “one”, “hundred”, “thousand”, “two”, “hundred”, “four”

Go to next sequence: Lil’ Jokes

# Lil’ Jokes

WORK IN PROGRESS

This is the place where we can write in any other things that we want to implement. 

