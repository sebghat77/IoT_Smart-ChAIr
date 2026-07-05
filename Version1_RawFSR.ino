//==================================================
// Smart Chair - Version 1
// Raw FSR Data Acquisition
// This version only returns sensor values.
// No posture decision is made here.
//==================================================


// ---------- Seat FSR Pins ----------
const int seatPins[4] = {34, 35, 32, 33};

// ---------- Backrest FSR Pins ----------
const int backPins[2] = {25, 26};


//==================================================
// Read all Seat FSR sensors
//
// Output:
// values[0] = Front Left
// values[1] = Front Right
// values[2] = Back Left
// values[3] = Back Right
//==================================================
void readSeatFSR(int values[])
{
    for(int i = 0; i < 4; i++)
    {
        values[i] = analogRead(seatPins[i]);
    }
}


//==================================================
// Read all Backrest FSR sensors
//
// Output:
// values[0] = Backrest Sensor 1
// values[1] = Backrest Sensor 2
//==================================================
void readBackrestFSR(int values[])
{
    for(int i = 0; i < 2; i++)
    {
        values[i] = analogRead(backPins[i]);
    }
}


void setup()
{
    Serial.begin(115200);
}


void loop()
{
    int seatValues[4];
    int backValues[2];

    // Read sensors
    readSeatFSR(seatValues);
    readBackrestFSR(backValues);

    // Print Seat values
    Serial.print("Seat: [");

    for(int i=0;i<4;i++)
    {
        Serial.print(seatValues[i]);

        if(i<3)
            Serial.print(", ");
    }

    Serial.print("]   ");

    // Print Backrest values
    Serial.print("Back: [");

    for(int i=0;i<2;i++)
    {
        Serial.print(backValues[i]);

        if(i<1)
            Serial.print(", ");
    }

    Serial.println("]");

    delay(500);
}
