/*
 * Copyright (c) 2021, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>  // printf

#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"
#include "sfa3x_i2c.h"

/*Para influxdb*/
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * TO USE CONSOLE OUTPUT (PRINTF) YOU MAY NEED TO ADAPT THE INCLUDE ABOVE OR
 * DEFINE IT ACCORDING TO YOUR PLATFORM:
 * #define printf(...)
 */

int main(void) {
    int16_t error = 0;

    sensirion_i2c_hal_init();

    error = sfa3x_device_reset();
    if (error) {
        // printf("Error resetting device: %i\n", error);
        return -1;
    }

    uint8_t device_marking[32];
    error =
        sfa3x_get_device_marking(&device_marking[0], sizeof(device_marking));
    if (error) {
        // printf("Error getting device marking: %i\n", error);
        return -1;
    }
    // printf("Device marking: %s\n", device_marking);

    // Start Measurement
    error = sfa3x_start_continuous_measurement();
    if (error) {
        // printf("Error executing sfa3x_start_continuous_measurement(): %i\n", error);
    }

    
    // Read Measurement

    float hcho;
    float humiditySFA30;
    float temperatureSFA30;

    sensirion_i2c_hal_sleep_usec(500000);

    error = sfa3x_read_measured_values(&hcho, &humiditySFA30, &temperatureSFA30);

    if (error) {
        // printf("Error executing sfa3x_read_measured_values(): %i\n", error);
    } else {
        // printf("Formaldehyde concentration: %.1f ppb\n", hcho);
        // printf("Relative humidity: %.2f %%RH\n", humiditySFA30);
        // printf("Temperature: %.2f °C\n", temperatureSFA30);
    }
    // Sample data, replace with the data you want to write to InfluxDB
    char *data = malloc(3072);
    char *tempStr = malloc(1024);
    strcpy(data, "hchoSensor,sensor_id=iotSFA ");
    sprintf(tempStr, "hcho_concentration=%.1f,", hcho);
    strcat(data, tempStr);
    sprintf(tempStr, "temperatureSFA30=%.2f,", temperatureSFA30);
    strcat(data, tempStr);
    sprintf(tempStr, "humiditySFA30=%.2f", humiditySFA30);
    strcat(data, tempStr);
    sprintf(tempStr, " %d", (int)time(NULL));
    strcat(data, tempStr);



    CURL *curl;
    CURLcode res;
    struct curl_slist *headers= NULL;
    curl = curl_easy_init();
    if (curl) {
        // Replace localhost:8086, org, and bucket with your InfluxDB Server
        // URL, Organization and Bucket.
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8086/api/v2/write?org=UCO&bucket=DatosSensores&precision=s");
        // Replace the API key
        char *token = TOKEN_INFLUX;
        // Concatenar "Authorization: Token " con el token
        char authorization_header[256]; 
        snprintf(authorization_header, sizeof(authorization_header), "Authorization: Token %s", token);

        headers = curl_slist_append(headers, authorization_header);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        headers = curl_slist_append(headers, "Accept: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        headers = curl_slist_append(headers,
                                    "Content-Type: text/plain; charset=utf-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        /* Get size of the POST data */
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(data));

        /* Pass in a pointer of data - libcurl will not copy */
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK) {
            // fprintf(stderr, "curl_easy_perform() failed: %s\n",
            // curl_easy_strerror(res));
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
    }

    error = sfa3x_stop_measurement();
    if (error) {
        // printf("Error executing sfa3x_stop_measurement(): %i\n", error);
    }

    return NO_ERROR;
}
