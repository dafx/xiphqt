#ifndef __ENCTHREAD_H__
#define __ENCTHREAD_H__

void encthread_init(void);
void encthread_addfile(char *file);
void encthread_setquality(int quality);
void encthread_setbitrate(int bitrate);

#endif
