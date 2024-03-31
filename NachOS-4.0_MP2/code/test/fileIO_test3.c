#include "syscall.h"

int main(void)
{
	char test[] = "abcdefghijklmnopqrstuvwxyz";

    int success;

	success= Create("file1.test"); // checked
    success= Create("file2.test");
    success= Create("file3.test");
    success= Create("file4.test");
    success= Create("file5.test");
    success= Create("file5.test");
    success= Create("file6.test");
    success= Create("file7.test");
    success= Create("file8.test");
    success= Create("file9.test");
    success= Create("file10.test");
    success= Create("file11.test"); // checked
    success= Create("file12.test");
    success= Create("file13.test");
    success= Create("file14.test");
    success= Create("file15.test");
    success= Create("file15.test");
    success= Create("file16.test");
    success= Create("file17.test");
    success= Create("file18.test");
    success= Create("file19.test");
    success= Create("file20.test");
    success= Create("file21.test");
    success= Create("file22.test");
    

    Open("file1.test"); // checked
    Open("file2.test");
    Open("file3.test");
    Open("file4.test");
    Open("file5.test");
    Open("file6.test");
    Open("file7.test");
    Open("file8.test");
    Open("file9.test");
    Open("file10.test");
    Open("file11.test"); // checked
    Open("file12.test");
    Open("file13.test");
    Open("file14.test");
    Open("file15.test");
    Open("file15.test");
    Open("file16.test");
    Open("file17.test");
    Open("file18.test");
    Open("file19.test");
    Open("file20.test");
    Open("file21.test");
    Open("file22.test");


	Halt();
}