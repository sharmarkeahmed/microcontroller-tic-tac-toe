// Final Project Code for PLAYER1
// NOTE: CODE IS INCOMPLETE

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
int PixelColorOff = strip.Color(0 , 0, 0); // Declaration & initialization of no LED color (off)

// Multidimensional arrays to store the state of the players. 
// true indicates LED turned on/player has put a tic or tac there, false indicates the player has not. 
bool P1State[3][3];
bool P2State[3][3];

// STATE VARIABLES ASSOCIATED WITH PUBLISHING
bool waitingToPublish = false; // If true, Particle Photon will wait at least 1 second before trying to publish something
bool publishNewChange = false; // If true, particle photon will publish a new change
bool publishGameReset = false; // If true, particle photon will publish a game reset
bool wait = false; // Once 1 second has elapsed, this will become false.

// OTHER STATE VARIABLES
bool resetGame = true;
bool player1Playing = true; // When the game first starts, player 1 will go first
bool player2Playing = false; // When the game first starts, player 2 will go second
bool updateGame = false; // Initially this is false, but if this is true the states of the other player has changed, so this microcontroller will also have to change as well. 

int ledTurnedOnP1; // LED to turn on (P1)
int ledTurnedOnP2; // LED that P2 Turned on

// Row and column changed by player 1
int p1RowChange;
int p1ColChange;

// Row and column changed by player 2
int p2RowChange;
int p2ColChange;


// Function Declarations
void startNewGame();
void turnOnLED(char key);
// Handler Function Declarations

unsigned long int waitingTime; // The amount of time to wait
void setup() {
    strip.begin(); // Initializes the strip of neopixel
    waitingTime = millis() + 1000; // Wait for 1s
    
}

void loop() {
    unsigned long int currentTime = millis();  // get the current time

    // In order to wait 1 second before publishing, a timer is used to avoid blocking code. NOTE: User cannot request a game reset for one second after pressing button. 
    if (currentTime > waitingTime) {
        // Ready to publish (if the photon is waiting to publish)
        wait = false;
        waitingTime += 1000;
    }


    if (waitingToPublish == false) {

        char key = keypad.getKey(); // This key will store the value that the user pressed

        // If the user presses the pound key, the game will be reset.
        if (key == '#') {
            publishGameReset = true; // Need to publish a game reset before actually resetting the game to make sure other particle photon resets their game board as well
            wait = true;
            waitingToPublish = true;
        }
        
        // If a user requests that the game should be reset, resetGame will be true for both particle photon's
        if (resetGame == true) {
            startNewGame();
            resetGame = false;
        }

        // Once P2 is done playing, the game will have to be reset
        if (updateGame == true) {
            strip.setPixelColor(ledTurnedOnP2, PixelColorBlue); // This will turn on the LED that was turned on by player 2
            P2State[p2RowChange][p2ColChange] = true;
            updateGame = false; // Done with updating the game
            strip.show(); // Updates LED colors
        }

        
        if ( (player1Playing == true ) ) {
            // This if statement will only run if player 1 is playing, otherwise must wait for other player to finish

            if (key) {
                // This if statement will only run if player 1 presses a key
                int iLED = turnOnThisLED(key); // iLED will store the iLED that should be turned on

                // If an iLED should turn on
                if (iLED != -1) {
                    strip.setPixelColor(iLED, PixelColorRed); // Turns on the iLED
                    waitingToPublish = true; // Photon will start waiting to publish
                    publishNewChange = true; // Ready to publish this new change!
                    wait = true; // Will ensure that at least 1 second will be waited for until particle publishing
                }

            } 
        }

    }
    else {
        if (wait == false) {
        // This contains every particle.publish() that this photon will do. 
            if (publishNewChange == true) {
                // FIXME: Must do a particle publish to both indicate it is now the other particle photon's turn to play the game.
                waitingToPublish = false;
            }
            else if (publishGameReset == true) {
                // FIXME: Must do a particle publish to indicate a game reset

            }
            strip.show(); // updates iLED's 
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
    // ------------
    // FIXME: Finish the rest of this code later on when 3x3 matrices are supported
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
