
double strtod(const char *s, char **scan_end)
{
	int sign = 1, i;
	double result = 0;
	double value;
	double mantissa = 0, divisor = 1;
	unsigned short power = 0;
	
	// check sign
	if (*s == '-') {
		sign = -1;
		s++;
	}
	
	// skip leading zeroes
	while (*s == '0') {
		s++;
	}
	
	// handle integer part
	while (*s <= '9' && *s >= '0') {
		value = *s++ - '0';
		result *= 10.0;
		result += value;
	}
	
	// find floating point and mantissa
	if (*s == '.') {
		s++;
		while (*s <= '9' && *s >= '0') {
		    value = *s++ - '0';
		    mantissa *= 10.0;
		    mantissa += value;
		    divisor *= 10.0;
		}
	}
	
	mantissa /= divisor;
	
	result += mantissa;
	result *= sign;
	
	// check exponent
	if (*s == 'e' || *s == 'E') {
		s++;
		
		if (*s == '-') {
			sign = -1;
			s++;
		} else if (*s == '+') {
			sign = 1;
			s++;
		} else {
			sign = 1;
		}
		
		while (*s <= '9' && *s >= '0') {
			value = *s++ - '0';
			power *= 10.0;
			power += value;
		}
	}
	
	if (sign > 0) {
		for (i = 0; i < power; i++) {
			result *= 10.0;
		}
	} else {
		for (i = 0; i < power; i++) {
			result /= 10.0;
		}
	}
	
	if (scan_end != 0) {
		*scan_end = s;
	}
	
	return result;
}

