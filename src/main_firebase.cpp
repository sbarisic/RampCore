#include <core2.h>
#include <Firebase_ESP_Client.h>
#include <firebase_secrets.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

#define printff(...)         \
    do                       \
    {                        \
        printf_time_now();   \
        printf(__VA_ARGS__); \
    } while (0)

FirebaseData fire_ramp1;
FirebaseAuth fire_auth;
FirebaseConfig fire_config;

const char *firebase_ramp1 = "/ramp1/open";

void setup_firebase()
{
    printff("Setting up firebase\n");

    fire_config.api_key = firebase_apikey;
    fire_config.database_url = firebase_database_url;
    fire_auth.user.email = firebase_email;
    fire_auth.user.password = firebase_password;

    fire_ramp1.setBSSLBufferSize(4096, 4096);
    Firebase.RTDB.setMaxErrorQueue(&fire_ramp1, 16);
    Firebase.RTDB.setMaxRetry(&fire_ramp1, 5);

    fire_config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

    Firebase.reconnectNetwork(false);
    Firebase.begin(&fire_config, &fire_auth);

    printff("Waiting for Firebase Ready ... ");

    while (!Firebase.ready())
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    printf("OK\n");
}

void core2_main_firebase(void *a)
{
    setup_firebase();

    int32_t last_check = core2_clock_bootseconds();
    bool resetFirebaseObject = false;

    while (true)
    {
        vTaskDelay(10);

        if (Firebase.ready())
        {
            if (resetFirebaseObject)
            {
                resetFirebaseObject = false;
                printff("Resetting %s\n", firebase_ramp1);
                Firebase.RTDB.setBool(&fire_ramp1, firebase_ramp1, false);
            }

            if (core2_clock_seconds_since(last_check) > 2)
            {
                last_check = core2_clock_bootseconds();

                bool Val;
                if (Firebase.RTDB.getBool(&fire_ramp1, firebase_ramp1, &Val))
                {
                    if (Val)
                    {
                        resetFirebaseObject = true;
                        core2_toggle_ramp();
                        printff("Firebase %s = true\n", firebase_ramp1);
                    }
                }
                else
                {
                    printff("Firebase.RTDB.getBool failed\n");
                }
            }
        }
    }
}
