/*
 *  alpmhelper.c
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Standard */
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

/* Non-standard */
#include <alpm.h>
#include <jansson.h>

/* Local */
#include "alpmhelper.h"
#include "util.h"

extern int opt_mask;
static pmdb_t *db_local;

void alpm_quick_init() {

    /* Should be parsing config here. For now, static declares */
    alpm_initialize();
    alpm_option_set_root("/");
    alpm_option_set_dbpath("/var/lib/pacman/");
    alpm_db_register_sync("core");
    alpm_db_register_sync("extra");
    alpm_db_register_sync("testing");
    alpm_db_register_sync("community");
    alpm_db_register_sync("community-testing");
    db_local = alpm_db_register_local();
}

static int is_foreign(pmpkg_t *pkg) {

    const char *pkgname = alpm_pkg_get_name(pkg);
    alpm_list_t *j;
    alpm_list_t *sync_dbs = alpm_option_get_syncdbs();

    int match = 0;
    for(j = sync_dbs; j; j = alpm_list_next(j)) {
        pmdb_t *db = alpm_list_getdata(j);
        pmpkg_t *findpkg = alpm_db_get_pkg(db, pkgname);
        if(findpkg) {
            match = 1;
            break;
        }
    }
    if(match == 0) {
        return 1 ;
    }
    return 0;
}

/* Equivalent of pacman -Qs or -Qm */
alpm_list_t *alpm_query_foreign() {

    alpm_list_t *i, *searchlist;
    alpm_list_t *ret = NULL;

    searchlist = alpm_db_get_pkgcache(db_local);

    if(searchlist == NULL) {
        return NULL;
    }

    for(i = searchlist; i; i = alpm_list_next(i)) {
        pmpkg_t *pkg = alpm_list_getdata(i);

        /* only weed out foreign pkgs if we called this
         * function with a NULL parameter
         */
        if (is_foreign(pkg)) { 
            ret = alpm_list_add(ret, pkg);
        }
    }

    return ret; /* This needs to be freed in the calling function */
}

pmdb_t *alpm_sync_search(alpm_list_t *target) {

    alpm_list_t *syncs, *i;

    syncs = alpm_option_get_syncdbs();

    for (i = syncs; i; i = alpm_list_next(i)) {
        pmdb_t *db = alpm_list_getdata(i);
        alpm_list_t *ret = alpm_db_search(db, target);

        if (! ret) {
            continue;
        } else {
            alpm_list_free(ret);
            return db; /* Great success! */
        }
    }

    return NULL; /* Failure */
}

int is_in_pacman(alpm_list_t *target) {

    pmdb_t *found_in;
    found_in = alpm_sync_search(target);

    if (found_in) {
        opt_mask & OPT_COLOR ? cfprint(1, alpm_list_getdata(target), WHITE) :
            printf("%s", (const char*)alpm_list_getdata(target));
        printf(" is available in "); 
        opt_mask & OPT_COLOR ? cfprint(1, alpm_db_get_name(found_in), YELLOW) :
            printf("%s", alpm_db_get_name(found_in));
        putchar('\n');

        return 1;
    }

    return 0;
}
