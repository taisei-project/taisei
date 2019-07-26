/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2019, p-sam <p-sam@d3vs.net>.
 */

#ifndef IGUARD_arch_switch_h
#define IGUARD_arch_switch_h

typedef void (*nxAtExitFn)(void);

void userAppInit(void);
void userAppExit(void);
int nxAtExit(nxAtExitFn fn);

void nxExit(int rc);

#endif // IGUARD_arch_switch_h
