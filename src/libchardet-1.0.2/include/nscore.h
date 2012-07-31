#ifndef INCLUDED_NSCORE_H
#define INCLUDED_NSCORE_H

typedef short PRInt16;
typedef unsigned short PRUint16;

typedef int PRInt32;
typedef unsigned PRUint32;

typedef int PRBool;
#define PR_TRUE 1
#define PR_FALSE 0

#define nsnull 0

typedef PRUint32 nsresult;
#define NS_OK 0
#define NS_ERROR_OUT_OF_MEMORY ((nsresult)(0x8007000eL))

#endif /* INCLUDED_NSCORE_H */
