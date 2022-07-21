#include <stdlib.h>
#include <stdio.h>
#include <grp.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

void check_root_group(struct group *grp) {
	assert(grp);

	printf("group name: %s\n", grp->gr_name);
	fflush(stdout);

	assert(grp->gr_gid == 0);
	assert(!strcmp(grp->gr_name, "root"));

	// Depending on your system, the root group may or may not contain the root user.
	if (grp->gr_mem[0] != NULL) {
		bool found = false;
		for (size_t i = 0; grp->gr_mem[i] != NULL; i++) {
			printf("group member: %s\n", grp->gr_mem[i]);
			fflush(stdout);

			if (!strcmp(grp->gr_mem[i], "root")) {
				found = true;
				break;
			}
		}
		assert(found);
	}
}

int main()
{
	struct group grp, *result = NULL;
	char *buf;
	size_t bufsize;
	int s;
	bool password = false;

	bufsize = sysconf(_SC_GETGR_R_SIZE_MAX);
	assert(bufsize > 0 && bufsize < 0x100000);

	buf = malloc(bufsize);
	assert(buf);

	s = getgrnam_r("root", &grp, buf, bufsize, &result);
	assert(!s);
	check_root_group(result);

	s = getgrgid_r(0, &grp, buf, bufsize, &result);
	assert(!s);
	check_root_group(result);

	result = getgrnam("root");
	check_root_group(result);

	result = getgrgid(0);
	check_root_group(result);

	result = getgrnam("this_group_doesnt_exist");
	assert(!result);
#ifndef USE_HOST_LIBC
	// This is not guaranteed.
	assert(errno == ESRCH);
#endif

	s = getgrnam_r("this_group_doesnt_exist", &grp, buf, bufsize, &result);
	assert(!result);
#ifndef USE_HOST_LIBC
	// This is not guaranteed.
	assert(s == ESRCH);
#endif

	errno = 0;
	setgrent();
	assert(errno == 0);

	result = getgrent();
	assert(result);
	assert(errno == 0);
	grp.gr_name = strdup(result->gr_name);
	if(result->gr_passwd) {
		password = true;
	}
	grp.gr_passwd = strdup(result->gr_passwd);
	grp.gr_gid = result->gr_gid;
	assert(grp.gr_name);
	if(password)
		assert(grp.gr_passwd);

	assert(grp.gr_name);
	if(password)
		assert(grp.gr_passwd);

	endgrent();
	assert(errno == 0);

	result = getgrent();
	assert(result);
	assert(errno == 0);
	assert(strcmp(result->gr_name, grp.gr_name) == 0);
	if(password)
		assert(strcmp(result->gr_passwd, grp.gr_passwd) == 0);
	assert(result->gr_gid == grp.gr_gid);

	free(grp.gr_name);
	if(password)
		free(grp.gr_passwd);
	free(buf);
}
