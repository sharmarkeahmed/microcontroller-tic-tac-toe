// Final Project Code for PLAYER1

/*
IMPORTANT: Import the neopixel and Keypad_Particle.h libraries in order to use this code
*/

#include "Particle.h"
#include "neopixel.h"
#include "Keypad_Particle.h"

// Neopixel Class Initializations:
// Sets pixel COUNT, PIN and TYPE for the neopixels
int PIXEL_PIN = D0;
int PIXEL_COUNT = 9; //There will be 9 pixels, from 0-8.
int PIXEL_TYPE = WS2812; // iLED's must all be of type WS2812!!
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

// Keypad initializations
// set up keypad buttons
const byte ROWS = 4;
const byte COLS = 3;

char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'},
};

byte rowPins[ROWS] = { D7, D6, D5, D4 };
byte colPins[COLS] = { D3, D2, D1 };

// create Keypad object variable called "keypad"
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// --- DECLARING GLOBAL VARIABLES ---

// Declaring & initializing iLED Colors. Note: WS2812 color code is in form strip.Color(Green, Red, Blue)
int PixelColorRed = strip.Color(0 , 255, 0); // Red LED color (to be used for PLAYER 1)
int PixelColorBlue = strip.Color(0, 0, 255); // Blue LED Color (to be used for PLAYER 2)
int PixelColorGreen = strip.Color(255, 0, 0); // Green LED Color (to be used in the case of a tie to blink all LED's a few times, see blinkAllLeds())
int PixelColorOff = strip.Color(0 , 0, 0); // Declaration & initialization of no LED color (off)

// Multidimensional arrays to store the state of the players. 
// true indicates LED turned on/player has put a tic or tac there, false indicates the player has not. 
bool P1State[3][3];
bool P2State[3][3];

// STATE VARIABLES ASSOCIATED WITH PUBLISHING
bool waitingToPublish = false; // If true, Particle Photon will wait to publish
bool publishNewChange = false; // If true, particle photon will publish a new change
bool publishGameReset = false; // If true, particle photon will publish a game reset
bool publishGameWin = false; // // If true, particle photon will publish a game win
bool wait = false; // Once 1 second has elapsed, this will become false. Used to ensure that particle photon waits one second to publish something. 
String winData; // Data to store information about the win.

// STATE VARIABLES ASSOCIATED WITH SUBSCRIPTION/HANDLER FUNCTIONS
int p2LED; // Stores the iLED that player 2 turned on
// Stores the row and column updated for player 2
int p2RowChange;
int p2ColChange;


// OTHER STATE VARIABLES
bool resetGame = true;
bool player1Playing = true; // When the game first starts, player 1 will go first
bool player2Playing = false; // When the game first starts, player 2 will go second
bool updateGame = false; // Initially this is false, but if this is true the states of the other player has changed, so this microcontroller will also have to change as well. 

// Row and column changed by player 1
int p1RowChange;
int p1ColChange;



// Winning LED Numbers will be stored in this array
int ledWinningNumbers[3];

int iLED; // Will hold the iLED number that is turned on


// Function Declarations
void startNewGame();
int turnOnThisLED(char key);
int detectWin();
void blinkLED();
void blinkAllLEDS();

// Declaration of handler functions
void P2StateChange(const char *event, const char *data);

// Handler Function Declarations

unsigned long int waitingTime; // The amount of time to wait
void setup() {
    strip.begin(); // Initializes the strip of neopixel
    waitingTime = millis() + 1000; // Wait for 1s
    Particle.subscribe("P2StateChange", P2StateChange);
}

void loop() {
    unsigned long int currentTime = millis();  // get the current time

    // In order to wait 1 second before publishing, a timer is used to avoid blocking code. Timer only matters when a publish is request (although timer will still continue to work so that its ready to go when needed)
    if (currentTime > waitingTime) {
        // One second has elapsed, we are now ready to publish (if the photon is waiting to publish)
        wait = false;
        waitingTime += 1000;
    }
    /* Note on requesting a publish: Please initialize wait to true and waitingToPublish to true in order to properly request a publish, otherwise weird things may begin to happen. 
     Likewise, waitingtoPublish should be initialized to false when done publishing something
     Also note the difference between wait and waitingToPublish, wait is initialied from true to false once one second has elapsed indicating a safe publish, waitingToPublish just blocks the code from doing other things until
     the publishing has happened. When waiting for a publish, the only things the photon will be doing will be checking for a reset request or seeing if the other photon has published anything.
    */

    char key = keypad.getKey(); // This key will store the value that the user pressed

    // If the user presses the pound key, the game will be reset.
        if (key == '#') {
            publishGameReset = true; // Need to publish a game reset before actually resetting the game to make sure other particle photon resets their game board as well
            wait = true; // Photon must now wait one second
            waitingToPublish = true; // Indicates that photon is waiting for a publish
        }

    if (waitingToPublish == false) { // If the photon is not waiting to publish something
        // If a user requests that the game should be reset, resetGame will be true for both particle photon's
        if (resetGame == true) {
            startNewGame();
            resetGame = false;
        }

        // Once P2 is done playing, the game will have to be reset

        if (updateGame == true) {
            strip.setPixelColor(p2LED, PixelColorBlue); // This will turn on the LED that was turned on by player 2
            P2State[p2RowChange][p2ColChange] = true;
            updateGame = false; // Done with updating the game
            strip.show(); // Updates LED colors
        }

        
        if ( (player1Playing == true ) ) {
            // This if statement will only run if player 1 is playing, otherwise must wait for other player to finish

            if (key) {
                // This if statement will only run if player 1 presses a key
                iLED = turnOnThisLED(key); // iLED will store the iLED that should be turned on
                // winStatus initialized to -2 if all iLED's on and both players lost, or 1 if a player has won a game, or -1 if neither players have yet to win for this round iteration
                int winStatus = detectWin(); 


                // If an iLED should turn on
                if (winStatus == 1) {
                    // A player has won a game. 

                    // Particle photon will wait to publish this.
                    winData = "W"; // The data will begin with W when published (see publishing section of loop() ), indicating a win from P1
                    publishGameWin = true;
                    waitingToPublish = true;
                    wait = true;
                }
                else if (winStatus == -2) {
                    // Neither player has won the game and all iLED's are on.

                    // Particle photon will wait to publish this
                    winData = "L"; // The data will begin with L when published (see publishing section of loop() ), indicating a loss for both players
                    publishGameWin = true;
                    waitingToPublish = true;
                    wait = true;
                }
                else if (iLED != -1) { // This else if statement will run if an iLED should be turned on and if nobody has won/lost yet
                    strip.setPixelColor(iLED, PixelColorRed); // Turns on the iLED
                    waitingToPublish = true; // Photon will start waiting to publish
                    publishNewChange = true; // Ready to publish this new change!
                    wait = true; // Will ensure that at least 1 second will be waited for until particle publishing
                }
                // Neopixels will not light up until publishing section of code, to ensure that both photons light up at the same time
            } 
        }

    }
    else { 
        // If the photon IS waiting to publish
        if (wait == false) { // If one second has elapsed (otherwise, continue waiting!)
        // This contains every particle.publish() that this photon could do. 
            if (publishNewChange == true) {
                String data;
                data = iLED;
                data += p1RowChange;
                data += p1ColChange;
                player1Playing = false;
                player2Playing = true;
                Particle.publish("P1StateChange", data);
                // Data will contain (in order) the iLED value, the p1RowChange, the p1ColChange value for P2 to update their valus.
                // We are done publishing, so must set these values to false as well
                waitingToPublish = false;
                publishNewChange = false;
            }
            else if (publishGameReset == true) {
                Particle.publish("P1ResetGameRequest", "");
                // Notify P2 that there was a reset game request
                resetGame = true; // P1 can now reset the game
                // We are done publishing, so must set these values to false as well
                waitingToPublish = false;
                publishGameReset = false;
            }
            else if (publishGameWin == true) {
                if (winData[0] == 'W') {
                    for (int i = 0; i < 3; i++) {
                    // If a win has occured, then the publish data will start with 'W' and contain the three winning numbers. That way, both photons can blink the winning numbers at the same time.
                    // If a win has not occured, winData will only have "L" when published.
                    winData += ledWinningNumbers[i];
                    }
                }
                Particle.publish("P1GameWin", winData);
                // We are done publishing, so must set these values to false as well
                waitingToPublish = false;
                publishGameWin = false;
                if (winData[0] == 'W') {
                    blinkLED(); // Blinks just the winning LED's
                }
                else {
                    blinkAllLEDS(); // Blink's all LED's (no one really won)
                }
            }
            strip.show(); // updates iLED's after the other photon has been notified (so both change colors at the same time).
        }   
        
    }
    
}


/*
Description: This function will reinitialize all state variables to false, and turn off all iLED's allowing for the game to essentially be reset. Because all state variables/arrays/variables are global, nothing needs to be passed by reference
Input:
    N/A
Output:
    N/A
*/
void startNewGame() {
    // For loop updates state variables of players
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            P1State[i][j] = false;
            P2State[i][j] = false;
        }
    }
    // For loop will turn off all the iLED's
    for (int i = 0; i < 9; i++) {
        strip.setPixelColor(i, PixelColorOff);
    }


    // Updating other state variables
    player1Playing = true; // When the game first starts, player 1 will go first
    player2Playing = false; // When the game first starts, player 2 will go second
    resetGame = false;
    updateGame = false;
    
    strip.show(); // Neopixels will change colors
    return;
}
/*
Description: This function will be used to determine, given a key press, which state variables to update, and which LED's (if any) to turn on. The function will return -1 if no LED should be turned on, or otherwise return the number LED to be turned on
    key - the key that was pressed by the user
Output:
    Returns the number of the iLED to turn on
    If an integer from 0-8 is returned, then that associated iLED is what should be turned on
    If an integer -1 is returned, no iLED should be turned on, and no state variables have been updated
*/
int turnOnThisLED(char key) {
     if (key == '1') {
        if (P1State[0][0] == false && P2State[0][0] == false) {
            P1State[0][0] = true;
        // (note: iLED 0 is associated with Player State[0][0] in multidimensional array for both photons)
        p1RowChange = 0;
        p1ColChange = 0;
        return 0; // Turn on iLED 0 

        }
        else {
            return -1;
        }
        
    }
    else if (key == '2') {
        if (P1State[0][1] == false && P2State[0][1] == false) {
            P1State[0][1] = true;
            p1RowChange = 0;
            p1ColChange = 1;
            return 1; // Turn on iLED 1 (note: iLED 1 is associated with Player State[0][1] in multidimensional arrays)
        }
        else {
            return -1;
        }
    }
    else if (key == '3') {
        if (P1State[0][2] == false && P2State[0][2] == false) {
            P1State[0][2] = true;
            p1RowChange = 0;
            p1ColChange = 2;
            return 2; // Turn on iLED 2 (note: iLED 2 is associated with Player State[0][2] in multidimensional arrays)
        }
        else {
            return -1;
        }
    }
    else if (key == '4') {
        if (P1State[1][0] == false && P2State[1][0] == false) {
            P1State[1][0] = true;
            p1RowChange = 1;
            p1ColChange = 0;
            return 5; // Turn on iLED 5 (note: iLED 5 is associated with Player State[1][0] in multidimensional arrays)

        }
        else {
            return -1;
        }
        
    }
    else if (key == '5') {
        if (P1State[1][1] == false && P2State[1][1] == false) {
            P1State[1][1] = true;
            p1RowChange = 1;
            p1ColChange = 1;
            return 4; // Turn on iLED 4 (note: iLED 4 is associated with Player State[1][1] in multidimensional arrays)
        }
        else {
            return -1;
        }
    }   
    else if (key == '6') {
        if (P1State[1][2] == false && P2State[1][2] == false) {
            P1State[1][2] = true;
            p1RowChange = 1;
            p1ColChange = 2;
            return 3; // Turn on iLED 3 (note: iLED 3 is associated with Player State[1][2] in multidimensional arrays)
        }
        else {
            return -1;
        }
        
    }    
    else if (key == '7') {
        if (P1State[2][0] == false && P2State[2][0] == false) {
            P1State[2][0] = true;
            p1RowChange = 2;
            p1ColChange = 0;
            return 6; // Turn on iLED 6 (note: iLED 6 is associated with Player State[2][0] in multidimensional arrays)
        }
        else {
            return -1;
        }
    }    
    else if (key == '8') {
        if (P1State[2][1] == false && P2State[2][1] == false) {
            P1State[2][1] = true;
            p1RowChange = 2;
            p1ColChange = 1;
            return 7; // Turn on iLED 7 (note: iLED 7 is associated with Player State[2][1] in multidimensional arrays)
        } 
        else {
            return -1;
        }
    }    
    else if (key == '9') {
        if (P1State[2][2] == false && P2State[2][2] == false) {
            P1State[2][2] = true;
            p1RowChange = 2;
            p1ColChange = 2;
            return 8; // Turn on iLED 8 (note: iLED 8 is associated with Player State[2][2] in multidimensional arrays)
        }
        else {
            return -1;
        }
        
    }  
    // -------------
    else { 
        return -1; // User did not press a button from 1-9, function will do nothing and return -1
    }

    return -1; // This will prevent some error from happening if for some reason the code escapes the if statements
}
/*Description: This function will detect whether or not the player has won the game.
Inputs:
    N/A
Outputs:
    1 - If player has won a game
    -1 - If player has not won a game
    -2 - If neither player wins after all LED's are on
Global Variables Used:
    ledWinningNumbers - changes the individual values of the array so that the program will know what LED's to blink later on. 
    P1State & P2State - uses the state of the two players to determine whether or not a win has occured
*/
int detectWin() {
    if(P1State[0][0] == P1State[0][1] && P1State[0][0] == P1State[0][2]){
 
            ledWinningNumbers[0] = 0;
            ledWinningNumbers[1] = 1;
            ledWinningNumbers[2] = 2;
            return 1;
 
        } else if(P1State[1][0] == P1State[1][1] && P1State[1][0] == P1State[1][2]){
 
            ledWinningNumbers[0] = 5;
            ledWinningNumbers[1] = 4;
            ledWinningNumbers[2] = 3;
            return 1;
 
        } else if(P1State[2][0] == P1State[2][1] && P1State[2][0] == P1State[2][2]){
 
            ledWinningNumbers[0] = 6;
            ledWinningNumbers[1] = 7;
            ledWinningNumbers[2] = 8;
            return 1;
 
        } else if(P1State[0][0] == P1State[1][0] && P1State[0][0] == P1State[2][0]){
 
            ledWinningNumbers[0] = 0;
            ledWinningNumbers[1] = 5;
            ledWinningNumbers[2] = 6;
            return 1;
 
        } else if(P1State[0][1] == P1State[1][1] && P1State[0][1] == P1State[2][1]){
 
            ledWinningNumbers[0] = 1;
            ledWinningNumbers[1] = 4;
            ledWinningNumbers[2] = 7;
            return 1;
 
        } else if(P1State[0][2] == P1State[1][2] && P1State[0][2] == P1State[2][2]){
 
            ledWinningNumbers[0] = 2;
            ledWinningNumbers[1] = 3;
            ledWinningNumbers[2] = 8;
            return 1;
 
        } else if(P1State[0][0] == P1State[1][1] && P1State[0][0] == P1State[2][2]){
 
            ledWinningNumbers[0] = 0;
            ledWinningNumbers[1] = 4;
            ledWinningNumbers[2] = 8;
            return 1;
 
        } else if(P1State[0][2] == P1State[1][1] && P1State[0][2] == P1State[2][0]){
 
            ledWinningNumbers[0] = 2;
            ledWinningNumbers[1] = 4;
            ledWinningNumbers[2] = 6;
            return 1;

        }
 else {
            
            // Determine whether or not the game has finished using for loops
            bool finishedGame = true; // This will detect whether or not all iLED's are on
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    if ( ( P1State[i][j] && P2State[i][j] ) == false ) {
                    finishedGame = false;
                    }
                }
            }
            if (finishedGame == true) {
                return -2; // The game has finished, nobody has won
            }
            else {
            return -1; // The game has yet to finish
            }
        }
}
/*
Description: To be used when a player has won a game, this function will blink the winning LED's three times . NOTE: Uses delay() blocking code to do the blinking
Inputs:
    N/A
Outputs:
    N/A
Global variables used:
strip.setPixelColor(iLed Number, Pixel Color) used to change the colors of the LED's
ledWinningNumbers array used to determine which iLed's to blink.
*/
void blinkLED() {
    // For loop to repeat this process three times
    for (int i = 0; i < 3; i++) {
    strip.setPixelColor(ledWinningNumbers[0],PixelColorRed);
    strip.setPixelColor(ledWinningNumbers[1],PixelColorRed);
    strip.setPixelColor(ledWinningNumbers[2],PixelColorRed);
    strip.show();
    delay(300); // Delay for .3 seconds
    strip.setPixelColor(ledWinningNumbers[0],PixelColorOff);
    strip.setPixelColor(ledWinningNumbers[1],PixelColorOff);
    strip.setPixelColor(ledWinningNumbers[2],PixelColorOff);
    strip.show();
    delay(300); // Delay for .3 seconds
    }
}
/*
Description: To be used when neither players have won a game, this function will blink all LED's three times. NOTE: Uses delay() blocking code to do the blinking. Will blink green when both lose
Inputs:
    N/A
Outputs:
    N/A
Global variables used:
strip.setPixelColor(iLed Number, Pixel Color) used to change the colors of the LED's
*/
void blinkAllLEDS() {
    // For loop to repeat this process three times
    for (int i = 0; i < 9; i++) {
    strip.setPixelColor(i,PixelColorRed);
    }
    strip.show();
    delay(300); // Delay for .3 seconds
    for (int i = 0; i < 9; i++) {
    strip.setPixelColor(i,PixelColorOff);
    }
    strip.show();
    delay(300); // Delay for .3 seconds
}

// --- Handler Functions ---

/*Description: This is the handler function associated with the particle publishing of a P2 State Change
*/
void P2StateChange(const char *event, const char *data) {
    // After P2 Publishes a state change, player 2 is no longer playing, and player 1 will play after this
    player2Playing = false;
    player1Playing = true;

    // ???FIXME: Change the state of P2 using such information, and turn on the associated iLED to blue.

    String p2LedTranslator = String(data[0]);
    String p2RowChangeTranslator = String(data[1]);
    String p2ColChangeTranslator = String(data[2]);

    //The above are intermediate variables to change the infromation recieved from the particle subscribe
    //to the below variables which are usable in 

    p2LED = p2LedTranslator.toInt();
    p2RowChange = p2RowChangeTranslator.toInt();
    p2ColChange = p2ColChangeTranslator.toInt();

    P1State[p2RowChange][p2ColChange] = true;
    strip.setPixelColor(p2LED, PixelColorBlue);

    strip.show();
    
    return;
}