/* mkdtemp.c */

#include <ulib/base/base.h>

#include <errno.h>

/* Very simple-minded mkdtemp() replacement */

extern U_EXPORT char* mkdtemp(char* rtemplate);

U_EXPORT char* mkdtemp(char* rtemplate)
{
   char* template;
   int i, oerrno, error;

   if (!(template = malloc(strlen(rtemplate) + 1))) return(NULL);

   for (error = 0, i = 0; i < 1000; ++i)
      {
      (void) strcpy(template, rtemplate);

      if (mktemp(template) == NULL)
         {
         error = 1;

         break;
         }

      error = mkdir(template, 0700);

      if (!error || errno != EEXIST) break;
      }

   oerrno = errno;

   if (*template) (void) strcpy(rtemplate, template);

   free(template);

   errno = oerrno;

   return (error ? NULL : rtemplate);
}
