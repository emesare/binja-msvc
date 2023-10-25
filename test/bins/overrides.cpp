#include <stdio.h>

class Animal
{
public:
	const char* name;

	virtual void make_sound() = 0;

	virtual void approach() { make_sound(); }

	void greet()
	{
		printf("You:\n");
		printf("Hello %s!\n", name);
		printf("%s:\n", name);
		make_sound();
	}
};

class Flying
{
public:
	int max_airspeed;

	virtual void fly() { puts("Up, up, and away!"); }
};

class Dog : public Animal
{
public:
	int bark_count;

	virtual void make_sound() override
	{
		puts("Woof!");
		bark_count++;
	}
};

class Cat : public Animal
{
public:
	int nap_count;

	virtual void make_sound() override { puts("Meow!"); }

	virtual void approach() override
	{
		if (nap_count)
			puts("Zzzz...");
		else
			make_sound();
	}

	virtual void nap() { nap_count++; }
};

class Lion : public Cat
{
public:
	virtual void make_sound() override { puts("Roar!"); }
};

class Bird : public Animal, public Flying
{
public:
	int song_length;

	Bird(int song_length)
	{
		this->name = "A bird";
		this->max_airspeed = 88;
		this->song_length = song_length;
	}

	virtual void make_sound() override
	{
		for (int i = 0; i < song_length; i++)
			puts("Tweet!");
	}

	virtual void approach() override { fly(); }
};

int main()
{
	Bird birdObj = Bird(5);
}