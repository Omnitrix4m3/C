#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>

#define NUM_STATES 50

//Stores climate info which is read in the struct
struct climate_info
{
    char code[3];
    long num_records;
    long double sum_temperature;
    long double sum_humidity;
    double max_temperature;
    time_t max_temperature_occurence;
    double min_temperature;
    time_t min_temperature_occurence;
    int lightning_strikes;
    int snow_cover;
    long double cloud_cover;
};

//Prevents an error where methods are introduced before being defined
void analyze_file(FILE *file, struct climate_info *states[], int num_states);
void print_report(struct climate_info *states[], int num_states);

//Main method
int main(int argc, char *argv[])
{
    //Returns an error if there is less than one file passed
    if (argc <= 1)
    {
        printf("Usage: %s tdv_file1 tdv_file2 ... tdv_fileN \n", argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; i++)
    {
        printf("Opening file: %s\n", argv[i]);
    }


    struct climate_info *states[NUM_STATES] = {NULL};

    for (int i = 1; i < argc; ++i)
    {
        FILE* fileInput = fopen(argv[i], "r"); //Allows the file to be read
        
        //Returns an error if the file is null
        if (fileInput == NULL)
        {
            printf("File is null!\n");
            continue;
        }
        
        analyze_file(fileInput, states, NUM_STATES); //Runs 'analyze_file' method to help analyze the data

        fclose(fileInput); //Prevents a resource leak by closing the file
    }

    print_report(states, NUM_STATES); //Returns an printed output of the files
    
    return 0; //Satisfies return condition in header
}

void analyze_file(FILE *file, struct climate_info **states, int num_states) //Goes through a file to analyze its data
{
    int index, found;

    const int line_sz = 100; 
    char line[line_sz];

    const char* delim = "\t\n";

    struct climate_info* s1;

    while (fgets(line, line_sz, file) != NULL) //Allows for tokenization of each line
    {
        char* token = strtok(line, delim);
        char* token_arr[9];

        int i = 0;
        
        while (token != NULL)
        {
            token_arr[i++] = token;
            token = strtok(NULL, delim);
        }

        found = 0;

        //Converts tokenized data stored as a string to double, int, time_t, etc
        char* code = token_arr[0];
        double temp = atof(token_arr[8]) * 1.8 - 459.67; //Standard conversion of Kelvin
        double sum_humidity = atof(token_arr[3]);
        int lightning_strikes = atoi(token_arr[6]);
        int snow_cover = atoi(token_arr[4]);
        double cloud_cover = atof(token_arr[5]);

        time_t time = atol(token_arr[1]);

        for (int i = 0; i < num_states; ++i) 
        {
            //Checks to see if the state is present and makes struct point to it
            if (states[i] != NULL && strcmp(code, states[i]-> code) == 0)
            {
                found = 1;

                s1 = states[i];
                index = i;
                
                break; //Breaks out of loop manually
            }
        }

        //Dynamically Allocates Memory in the case the state is not found and copies over its instance data
        if (found == 0)
        {
            s1 = (struct climate_info *) malloc(sizeof(struct climate_info));
            
            strcpy(s1 -> code, code);

            s1 -> num_records = 1;
            s1 -> sum_humidity = sum_humidity;
            s1 -> sum_temperature = temp;
            s1 -> max_temperature = temp;
            s1 -> max_temperature_occurence = time / 1000;
            s1 -> min_temperature = temp;
            s1 -> min_temperature_occurence = time / 1000;
            s1 -> lightning_strikes = lightning_strikes;
            s1 -> snow_cover = snow_cover;
            s1 -> cloud_cover = cloud_cover;
            
            for (int i = 0; i < num_states; ++i)
            {
                if (states[i] == NULL)
                {
                    states[i] = s1;
                    break;
                }
            }
        }

        //For all other cases if the state is present, the data is augmented to, not overwritten
        else
        {
            states[index] -> num_records += 1;
            states[index] -> sum_humidity += sum_humidity;
            states[index] -> sum_temperature += temp;
            states[index] -> lightning_strikes += lightning_strikes;
            states[index] -> snow_cover += snow_cover;
            states[index] -> cloud_cover += cloud_cover;

            //Algorithm that finds the greatest maximum temperature
            if (states[index] -> max_temperature < temp)
            {
                states[index] -> max_temperature = temp;
                states[index] -> max_temperature_occurence = time / 1000;
            }
            
            //Algorithm that finds the greatest minimum temperature
            if (states[index] -> min_temperature > temp)
            {
                states[index] -> min_temperature = temp;
                states[index] -> min_temperature_occurence = time / 1000;
            }
        }
    }
}

//Produces a visible output of the data analysis
void print_report(struct climate_info *states[], int num_states)
{
    printf("States found: ");

    for (int i = 0; i < num_states; ++i)
    {
        if (states[i] != NULL)
        {
            struct climate_info* s1 = states[i];
            printf("%s ", s1 -> code);
        }
    }

    printf("\n");

    for (int i = 0; i < num_states; i++)
    {
        if (states[i] != NULL)
        {
            struct climate_info* current_state  = states[i];

            float avg_sum_humidity = ((current_state -> sum_humidity * 1.0) / (current_state -> num_records)); //Calculates average humidity
            float avg_temp = ((current_state -> sum_temperature * 1.0) / (current_state -> num_records)); //Calculates average temperature

            //Formatting
            printf("-- State: %s --\n", current_state -> code);
            printf("Number of Records: %lu\n", current_state -> num_records);
            printf("Average Humidity: %0.1f%%\n", (avg_sum_humidity));            
            printf("Average Temperature: %0.1fF\n", (avg_temp));
            printf("Max Temperature: %0.1fF\n", current_state -> max_temperature);
            printf("Max Temperature on: %s", ctime(&(current_state -> max_temperature_occurence)));
            printf("Min Temperature: %0.1fF\n", current_state -> min_temperature);
            printf("Min Temperature on: %s", ctime(&(current_state -> min_temperature_occurence)));
            printf("Lightning Strikes: %d\n", current_state -> lightning_strikes);
            printf("Records with Snow Cover: %d\n", current_state -> snow_cover);
            printf("Average Cloud Cover: %0.1Lf%%", current_state -> cloud_cover * 1.0 / current_state -> num_records);
        }
    }
}
