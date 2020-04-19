#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

//Checks the integrity of a user generated password with more constraints
bool isaStrongPassword(const char* username, const char* password)
{	
	//Instance data
	int i = 0, j = 0, count = 0, step = 0;

	bool minLength = false, oneUpper = false, oneLower = false, oneDigit = false, containsString = false, validChars = false;

	//Goes through the array to satisfy each of the conditions
	if (strlen(password) > 8)
	{
		minLength = true;

		for (int i = 0; i < strlen(password); i++)
		{
			if (islower(password[i]))
			{
				oneLower = true;
			}

			if (isupper(password[i]))
			{
				oneUpper = true;
			}

			if (isdigit(password[i]))
			{
				oneDigit = true;
			} 
		}
	}
	
    //Looks for a sequence of characters in the password
	while(step < strlen(password) && containsString == false)
    {
		if (isdigit(password[step]))
        {
            count = 0;
        }
		
		if (isalpha(password[step]))
		{
            count++;
        }

        if (count == 4)
        {
            containsString = true;
        }
		
		step++;
    }
	
	//Checks to see if the password string shares elements with the username
	for (i = 0; password[i]; i++)
    {
        if (tolower(username[j]) == tolower(password[i]))
        {
            j = 0;

			for (int index = i; username[j] && password[index]; index++)
            {
                if (tolower(username[j]) != tolower(password[index]))
                {
                    break;
                }

				j++;
            }

            if (!tolower(username[j]))
            {
                return false;
            }
        }
    }

    //Checks to see if each character in the string is either a letter or number
	for (i = 0; i < strlen(password); i++)
    {
        if (isalpha(password[i]) == false && isdigit(password[i]) == false)
        {
			validChars = false;
        }

		else
		{
			validChars = true;
		}
    }

    //If any cases fail, the password is weak
	if (minLength == false || oneUpper == false || oneLower == false || oneDigit == false || containsString == false || validChars == false)
    {
        return false;
    }
	
	else
	{
		return true;
	}
}	


//Checks the integrity of a randomly generated password with less constraints
bool isStrongDefaultPassword(const char* username, const char* password)
{
	//Instance data
	int i = 0, j = 0, count = 0, step = 0;

	bool minLength = false, oneUpper = false, oneLower = false, oneDigit = false, maxLength = false;

	//Goes through the array to satisfy each of the conditions
	if (strlen(password) > 8 && strlen(password) < 15)
	{
		minLength = true;
		maxLength = true;

		for (int i = 0; i < strlen(password); i++)
		{
			if (islower(password[i]))
			{
				oneLower = true;
			}

			if (isupper(password[i]))
			{
				oneUpper = true;
			}

			if (isdigit(password[i]))
			{
				oneDigit = true;
			} 
		}
	}
	
	//Checks to see if the password string shares elements with the username
    for (i = 0; password[i]; i++)
    {
        if (tolower(username[j]) == tolower(password[i]))
        {
            j = 0;

			for (int index = i; username[j] && password[index]; index++)
            {
                if (tolower(username[j]) != tolower(password[index]))
                {
                    break;
                }

				j++;
            }

            if (!tolower(username[j]))
            {
                return false;
            }
        }
    }

	//If any cases fail, the password is weak
    if (minLength == false || oneUpper == false || oneLower == false || oneDigit == false || maxLength == false)
    {
        return false;
    }
	
	else
	{
		return true;
	}
}

//Generates a random password within the preset boundaries
void generateDefaultPassword(char* default_password, const char* username)
{
	bool isRunning = false;
	char characterArray[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	
	srandom(time(0));	
	
	//Keeps trying to find a random password that satisfies integrity requirements
	while (isRunning == false)
    {
		srandom(time(0));

		int length = rand() % ((20 + 1) - 1) + 1;

        for (int i = 0; i < length; i++)
        {
            char randChar = characterArray[random() % 62];
		    default_password[i] = randChar;
        }

        default_password[length] = '\0';
	    isRunning = isStrongDefaultPassword(username, default_password);
	}

	printf("Generated default password: %s", default_password);
}

//Tests the methods by requiring user input
int main(void)
{
	char username[20], password[20];
    
	printf("Enter a username: ");
    scanf("%s", username);
    printf("Enter a password: ");
    scanf("%s", password);

	if (isaStrongPassword(username, password))
    {
        printf("Strong password!\n");
    }

    else    
    {
        printf("Your password is weak. Try again!\n");
    }

	printf("Generating a default password...\n");
	generateDefaultPassword(password, username);
	
	return 0;
}