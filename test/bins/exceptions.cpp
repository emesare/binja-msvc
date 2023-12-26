#include <iostream>

void except_int()
{
	try
	{
		puts("Throwing an integer exception!");
		throw 42;
	}
	catch (int i)
	{
		auto y = i + 33;
	}

	puts("After throwing an integer exception!");
}

void except_std()
{
	try
	{
		puts("Throwing an out-of-range exception!");
		throw std::out_of_range("Out of range!");
	}
	catch (const std::exception& e)  // caught by reference to base
	{
		puts("What is next!");
		puts(e.what());
	}

	puts("After throwing an out-of-range exception!");
}

int main()
{
	except_int();
	except_std();
}