#include "matcher.h"

//Returns the previous char using pointer arithmetic
char charLast(char *str)
{
	return *(str - sizeof(char));
}

//Returns the next char using pointer arithmetic
char charNext(char *str)
{
	return *(str + sizeof(char));
}

//Identifies if the char type argument is a special character or not
int isSpecialChar(char c)
{
	switch (c)
	{
		case '.':
			return 1;

		case '\\':
			return 2;
		
		case '+':
			return 3;
		
		case '?':
			return 4;
		
		default:
			return 0;
	}
}

//Checks to see if '+' occurs anywhere in the String by checking surrounding char values
int plusOccurence(char *pattern)
{
	if (charNext(pattern) == '+')
	{
		return 1;
	}

	return 0;
}

//Checks to see if '?' occurs anywhere in the String by checking surrounding char values
int questionOccurence(char *pattern)
{
	if (charNext(pattern)  == '?')
	{
		return 1;
	}

	return 0;
}

//Checks to see if '\' occurs anywhere in the String by checking surrounding char values
int backslashOccurence(char *pattern)
{
	if (charLast(pattern) == '\\')
	{
		return 1;
	}

	return 0;
}

/**
 * Returns true if partial_line matches pattern, starting from
 * the first char of partial_line.
 */
int matches_leading(char *partial_line, char *pattern)
{
	if (*partial_line == *pattern && !isSpecialChar(*pattern))
	{
		return 1;
	}

	//Checks for a special case
	if (*pattern == '.' && !backslashOccurence(pattern))
	{
		return 1;
	}

	//Checks for a backslash
	if (*partial_line == *pattern && isSpecialChar(*pattern) && backslashOccurence(pattern))
	{
		return 1;
	}

	//Also checks for a backslash
	if (*pattern == '\\')
	{
		return matches_leading(partial_line, pattern + sizeof(char));
	}

	//Checks for the occurence of a question mark
	if (questionOccurence(pattern))
	{
		return 1;
	}

	return 0;
}

//Implementation of restricted rgrep to be used by main method
int rgrep_matches(char *line, char *pattern)
{
	static int depth = 0;

	//Base case which checks to see if the iterator has reached the end of the string
	if (*pattern == '\0')
	{
		//Checks for a new line character	
		if (*line == '\n')
		{
			pattern -= depth * sizeof(char);
		}

		depth = 0;

		return 1;
	}
	
	if (*line == '\0')
	{
		return 0;
	}

	//Resets the pattern for new line	
	if (*line == '\n')
	{
		pattern -= depth * sizeof(char);
		depth -= depth;

		return 0;
	}

	//Checks for backslash
	if ((*pattern == '\\') && isSpecialChar(charNext(pattern)))
	{
		pattern += sizeof(char);
		depth++;
	}

	//Checks for a match using helper method
	if (matches_leading(line, pattern))
	{
		//Checks for '+' special character
		if (plusOccurence(pattern))
		{
			//Goes through all possible combinations
			int charsPrior = 1;
			
			if (*pattern == *(pattern + 2 * sizeof(char)))
			{
				charsPrior = 2;
			}
			
			while (*(line + charsPrior * sizeof(char)) == *pattern && !(*pattern == '.' && !backslashOccurence(pattern)))
			{
				line += sizeof(char);
			}

			//Allows for the exception of a '.+' occurence
			if (*pattern == '.' && backslashOccurence(pattern))
			{
				char specialCase = *line;
				
				while (*(line + charsPrior * sizeof(char)) == specialCase)
				{
					line += sizeof(char);
				}			
			}

			if (*pattern == '.' && !backslashOccurence(pattern))
			{
				while (!matches_leading(line + charsPrior * sizeof(char), pattern + 2 * sizeof(char)))
				{
					line += sizeof(char);
				
					if ((*line == '\0' || *line == '\n') && *(pattern + 2 * sizeof(char)) != '\0')
					{
						return 0;
					}
				}
			}

			pattern += sizeof(char);
			depth++;
		}

		//Checks for '?' special character
		if (questionOccurence(pattern))
		{
			//Goes through all possible combinations
			if (*pattern == '.' && !backslashOccurence(pattern) && backslashOccurence(pattern + 3 * sizeof(char)) && *line == *(pattern + 3 * sizeof(char)) && *(line + sizeof(char)) != *(pattern + 3 * sizeof(char)))
			{
				depth += 4;

				return rgrep_matches(line + sizeof(char), pattern + 4 * sizeof(char));
			
			}
			
			else if (*pattern == '.' && !backslashOccurence(pattern) && *line == *(pattern + 2 * sizeof(char)))
			{
				depth += 2;

				return rgrep_matches(line, pattern + 2 * sizeof(char));
			}
			
			else if (*pattern == '.' && !backslashOccurence(pattern) && *line != *(pattern + 2 * sizeof(char)))
			{
				depth += 2;

				return rgrep_matches(line + sizeof(char), pattern + 2 * sizeof(char));
			
			}
			
			else if (*line == *pattern && *line == *(pattern + 2 * sizeof(char)))
			{
				depth += 2;

				return rgrep_matches(line, pattern + 2 * sizeof(char));
			}

			else if (*line != *pattern && *line != *(pattern + 2 * sizeof(char)))
			{
				return 0;
			}

			else if (!(*pattern == '.' && !backslashOccurence(pattern)) || *line != *pattern)
			{
				line -= sizeof(char);
			}

			pattern += sizeof(char);
			depth++;
		}

		pattern += sizeof(char);
		depth++;
	}

	//Allows for contiguous matches only
	else if (depth != 0 && *(pattern - depth * sizeof(char)) != '\0')
	{
		pattern -= depth * sizeof(char);
		depth -= depth;
	}

	//if(*line != '\n')
		line += sizeof(char);

	

	//Recursive call to allow for iteraton
	return rgrep_matches(line, pattern);
}
