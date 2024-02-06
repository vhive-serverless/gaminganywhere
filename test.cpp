#include <stdio.h>
#include <string.h>
#include <iostream>

using namespace std;
// Example structure
struct MyStruct {
    int intValue;
    char* stringValue;
    float floatValue;
};

// Serialization function
void serialize(const struct MyStruct* data, char* buffer, size_t bufferSize) {
    // Check buffer size
    if (bufferSize < sizeof(struct MyStruct)) {
        printf("Error: Insufficient buffer size for serialization\n");
        return;
    }

    // Copy the structure into the buffer
    memcpy(buffer, data, sizeof(struct MyStruct));
}

// Deserialization function
void deserialize(struct MyStruct* data, const char* buffer, size_t bufferSize) {
    // Check buffer size
    if (bufferSize < sizeof(struct MyStruct)) {
        printf("Error: Insufficient buffer size for deserialization\n");
        return;
    }

    // Copy data from buffer to structure
    memcpy(data, buffer, sizeof(struct MyStruct));
}

int main() {
    // Example structure instance
    struct MyStruct originalData = {42, "Helloioserjgoiesrjgisdefjcidfvciujoiergjisrfgbhksufrdhvbhskdueryfgvhndkfscjv nskurfidhjng ibguheribgr heiunsoevrinfjiudrngjfs euigiinhwserigunhvrdjv", 3.14};

    // Buffer for serialization
    char serializedBuffer[sizeof(struct MyStruct)];

    cout << sizeof(struct MyStruct) << endl;
    cout << sizeof(originalData.stringValue) << endl;

    // Serialize the structure
    serialize(&originalData, serializedBuffer, sizeof(serializedBuffer));

    // Deserialized structure
    struct MyStruct deserializedData;

    // Deserialize the structure
    deserialize(&deserializedData, serializedBuffer, sizeof(serializedBuffer));

    // Output the results
    printf("Original Data: %d %f %s\n", originalData.intValue, originalData.floatValue, originalData.stringValue);
    printf("Deserialized Data: %d %f %s\n", deserializedData.intValue, deserializedData.floatValue, deserializedData.stringValue);

    return 0;
}