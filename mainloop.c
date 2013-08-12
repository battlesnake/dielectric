#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include "wave.h"
#include "runsweep.h"
#include "mainloop.h"

int MainLoop(struct WAVEPARAMS* w, float fmin, float fmax, float sweepduration) {
	char filename[MAX_PATH + 1], c;
	int isconsole = _isatty(_fileno(stdin));
	do {

		/* Get filename */
		printf("Run name: \t");
		if (scanf("%250s", filename) < 1) 
			return 0;

		/* Wait for user to initiate sweep */
		if (isconsole) {
			puts("");
			fflush(stdin);
			puts("Press <enter> to run");
			getchar();
		}

		/* Sweep! */
		if (!RunSweep(w, fmin, fmax, sweepduration, filename))
			puts("Error: operation failed.");

		/* Again? */
		puts("");
		printf("Another (y/n)? ");
		do {
			c = toupper(getchar());
		} while (c != 'Y' && c != 'N');
		puts("");
	} while (c == 'Y' && isconsole);
	return 1;
}
