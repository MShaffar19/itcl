/*
 * itclngInt.h --
 *
 * This file contains internal definitions for the C-implemented part of
 * Itclng
 *
 * Copyright (c) 2007 by Arnulf P. Wiedemann
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: itclngInt.h,v 1.1.2.1 2008/01/12 18:45:32 wiede Exp $
 */

#include <string.h>
#include <ctype.h>
#include <tcl.h>
#include <tclOO.h>
#include "itclngMigrate2TclCore.h"
#include "itclngTclIntStubsFcn.h"
#include "itclngNeededFromTclOO.h"
#include "itclng.h"

/*
 * Used to tag functions that are only to be visible within the module being
 * built and not outside it (where this is supported by the linker).
 */

#ifndef MODULE_SCOPE
#   ifdef __cplusplus
#       define MODULE_SCOPE extern "C"
#   else
#       define MODULE_SCOPE extern
#   endif
#endif

/*
 * Since the Tcl/Tk distribution doesn't perform any asserts,
 * dynamic loading can fail to find the __assert function.
 * As a workaround, we'll include our own.
 */

#undef  assert
#define DEBUG 1
#ifndef  DEBUG
#define assert(EX) ((void)0)
#else
#define assert(EX) (void)((EX) || (Itclng_Assert(STRINGIFY(EX), __FILE__, __LINE__), 0))
#endif  /* DEBUG */

#define ITCLNG_INTERP_DATA "itclng_data"

/*
 * Convenience macros for iterating through hash tables. FOREACH_HASH_DECLS
 * sets up the declarations needed for the main macro, FOREACH_HASH, which
 * does the actual iteration. FOREACH_HASH_VALUE is a restricted version that
 * only iterates over values.
 */

#define FOREACH_HASH_DECLS \
    Tcl_HashEntry *hPtr;Tcl_HashSearch search
#define FOREACH_HASH(key,val,tablePtr) \
    for(hPtr=Tcl_FirstHashEntry((tablePtr),&search); hPtr!=NULL ? \
            ((key)=(void *)Tcl_GetHashKey((tablePtr),hPtr),\
            (val)=Tcl_GetHashValue(hPtr),1):0; hPtr=Tcl_NextHashEntry(&search))
#define FOREACH_HASH_VALUE(val,tablePtr) \
    for(hPtr=Tcl_FirstHashEntry((tablePtr),&search); hPtr!=NULL ? \
            ((val)=Tcl_GetHashValue(hPtr),1):0;hPtr=Tcl_NextHashEntry(&search))

/*
 * What sort of size of things we like to allocate.
 */

#define ALLOC_CHUNK 8

#define ITCLNG_VARIABLES_NAMESPACE "::itclng::internal::variables"
#define ITCLNG_COMMANDS_NAMESPACE "::itclng::internal::commands"

typedef struct ItclngArgList {
    struct ItclArgList *nextPtr;        /* pointer to next argument */
    Tcl_Obj *namePtr;           /* name of the argument */
    Tcl_Obj *defaultValuePtr;   /* default value or NULL if none */
} ItclngArgList;

/*
 *  Common info for managing all known objects.
 *  Each interpreter has one of these data structures stored as
 *  clientData in the "itcl" namespace.  It is also accessible
 *  as associated data via the key ITCL_INTERP_DATA.
 */
struct ItclngClass;
struct ItclngObject;
struct ItclngMemberFunc;
struct ItclngDelegatedOptionInfo;
struct ItclngDelegatedMethodInfo;

typedef struct ItclngObjectInfo {
    Tcl_Interp *interp;             /* interpreter that manages this info */
    Tcl_HashTable objects;          /* list of all known objects */
    Tcl_HashTable classes;          /* list of all known classes */
    Tcl_HashTable namespaceClasses; /* maps from nsPtr to iclsPtr */
    Tcl_HashTable procMethods;      /* maps from procPtr to mFunc */
    int protection;                 /* protection level currently in effect */
    int useOldResolvers;            /* whether to use the "old" style
                                     * resolvers or the CallFrame resolvers */
    Itclng_Stack clsStack;            /* stack of class definitions currently
                                     * being parsed */
    Itclng_Stack contextStack;        /* stack of call contexts */
    Itclng_Stack constructorStack;    /* stack of constructor calls */
    struct ItclngObject *currIoPtr;   /* object currently being constructed
                                     * set only during calling of constructors
				     * otherwise NULL */
    Tcl_ObjectMetadataType *class_meta_type;
                                    /* type for getting the Itcl class info
                                     * from a TclOO Tcl_Object */
    Tcl_ObjectMetadataType *object_meta_type;
                                    /* type for getting the Itcl object info
                                     * from a TclOO Tcl_Object */
    Tcl_Object clazzObjectPtr;      /* the root object of Itcl */
    Tcl_Class clazzClassPtr;        /* the root class of Itcl */
    struct EnsembleInfo *ensembleInfo;
    struct ItclngClass *currContextIclsPtr;
                                    /* context class for delegated option
                                     * handling */
    int currClassFlags;             /* flags for the class just in creation */
    int unparsedObjc;               /* number options not parsed by 
                                       ItclExtendedConfigure/-Cget function */
    Tcl_Obj **unparsedObjv;         /* options not parsed by
                                       ItclExtendedConfigure/-Cget function */
} ItclngObjectInfo;

/*
 *  Representation for each [incr Tcl] class.
 */
typedef struct ItclngClass {
    Tcl_Obj *namePtr;             /* class name */
    Tcl_Obj *fullNamePtr;         /* fully qualified class name */
    Tcl_Interp *interp;           /* interpreter that manages this info */
    Tcl_Namespace *nsPtr;         /* namespace representing class scope */
    Tcl_Command accessCmd;        /* access command for creating instances */

    struct ItclngObjectInfo *infoPtr;
                                  /* info about all known objects
				   * and other stuff like stacks */
    Itclng_List bases;              /* list of base classes */
    Itclng_List derived;            /* list of all derived classes */
    Tcl_HashTable heritage;       /* table of all base classes.  Look up
                                   * by pointer to class definition.  This
                                   * provides fast lookup for inheritance
                                   * tests. */
    Tcl_Obj *initCode;            /* initialization code for new objs */
    Tcl_HashTable variables;      /* definitions for all data members
                                     in this class.  Look up simple string
                                     names and get back ItclVariable* ptrs */
    Tcl_HashTable options;        /* definitions for all option members
                                     in this class.  Look up simple string
                                     names and get back ItclOption* ptrs */
    Tcl_HashTable components;     /* definitions for all component members
                                     in this class.  Look up simple string
                                     names and get back ItclComponent* ptrs */
    Tcl_HashTable functions;      /* definitions for all member functions
                                     in this class.  Look up simple string
                                     names and get back ItclMemberFunc* ptrs */
    Tcl_HashTable delegatedOptions; /* definitions for all delegated options
                                     in this class.  Look up simple string
                                     names and get back
				     ItclDelegatedOptionInfo * ptrs */
    Tcl_HashTable delegatedFunctions; /* definitions for all delegated methods
                                     or procs in this class.  Look up simple
				     string names and get back
				     ItclDelegatedMethodInfo * ptrs */
    Tcl_HashTable methodVariables; /* definitions for all methodvariable members
                                     in this class.  Look up simple string
                                     names and get back
				     ItclMethodVariable* ptrs */
    int numInstanceVars;          /* number of instance vars in variables
                                     table */
    Tcl_HashTable classCommons;   /* used for storing variable namespace
                                   * string for Tcl_Resolve */
    Tcl_HashTable resolveVars;    /* all possible names for variables in
                                   * this class (e.g., x, foo::x, etc.) */
    Tcl_HashTable resolveCmds;    /* all possible names for functions in
                                   * this class (e.g., x, foo::x, etc.) */
    Tcl_HashTable contextCache;   /* cache for function contexts */
    struct ItclngMemberFunc *constructor;
                                  /* the class constructor or NULL */
    struct ItclngMemberFunc *destructor;
                                  /* the class destructor or NULL */
    struct ItclngMemberFunc *constructorInit;
                                  /* the class constructor init code or NULL */
    Tcl_Resolve *resolvePtr;
    Tcl_Obj *widgetClassPtr;      /* class name for widget if class is an
                                   * ::itcl::widget */
    Tcl_Object oPtr;		  /* TclOO class object */
    Tcl_Class  clsPtr;            /* TclOO class */
    int numCommons;               /* number of commons in this class */
    int numVariables;             /* number of variables in this class */
    int unique;                   /* unique number for #auto generation */
    int flags;                    /* maintains class status */
} ItclngClass;

#define ITCLNG_CLASS		        0x0001000
#define ITCLNG_CLASS_NS_TEARDOWN        0x2000000

typedef struct ItclngHierIter {
    ItclngClass *current;           /* current position in hierarchy */
    Itclng_Stack stack;             /* stack used for traversal */
} ItclngHierIter;

/*
 *  Representation for each [incr Tcl] object.
 */
typedef struct ItclngObject {
    ItclngClass *iclsPtr;          /* most-specific class */
    Tcl_Command accessCmd;       /* object access command */

    Tcl_HashTable* constructed;  /* temp storage used during construction */
    Tcl_HashTable* destructed;   /* temp storage used during destruction */
    Tcl_HashTable objectVariables;
                                 /* used for storing Tcl_Var entries for
				  * variable resolving, key is ivPtr of
				  * variable, value is varPtr */
    Tcl_HashTable objectOptions; /* definitions for all option members
                                     in this object. Look up option namePtr
                                     names and get back ItclOption* ptrs */
    Tcl_HashTable objectMethodVariables;
                                 /* definitions for all methodvariable members
                                     in this object. Look up methodvariable
				     namePtr names and get back
				     ItclMethodVariable* ptrs */
    Tcl_HashTable objectDelegatedOptions;
                                  /* definitions for all delegated option
				     members in this object. Look up option
				     namePtr names and get back
				     ItclOption* ptrs */
    Tcl_HashTable objectDelegatedFunctions;
                                  /* definitions for all delegated function
				     members in this object. Look up function
				     namePtr names and get back
				     ItclMemberFunc * ptrs */
    Tcl_HashTable contextCache;   /* cache for function contexts */
    Tcl_Obj *namePtr;
    Tcl_Obj *varNsNamePtr;
    Tcl_Object oPtr;             /* the TclOO object */
    Tcl_Resolve *resolvePtr;
    int flags;
} ItclngObject;

#define ITCLNG_OBJECT_IS_DELETED           0x01
#define ITCLNG_OBJECT_IS_DESTRUCTED        0x02
#define ITCLNG_OBJECT_IS_RENAMED           0x04
#define ITCLNG_TCLOO_OBJECT_IS_DELETED     0x10
#define ITCLNG_OBJECT_NO_VARNS_DELETE      0x20
#define ITCLNG_OBJECT_SHOULD_VARNS_DELETE  0x40
#define ITCLNG_CLASS_NO_VARNS_DELETE       0x100
#define ITCLNG_CLASS_SHOULD_VARNS_DELETE   0x200
#define ITCLNG_CLASS_DELETE_CALLED         0x400
#define ITCLNG_CLASS_DELETED               0x800

#define ITCLNG_IGNORE_ERRS  0x002  /* useful for construction/destruction */

/*
 *  Implementation for any code body in an [incr Tcl] class.
 */
typedef struct ItclMemberCode {
    int flags;                  /* flags describing implementation */
    int argcount;               /* number of args in arglist */
    int maxargcount;            /* max number of args in arglist */
    Tcl_Obj *usagePtr;          /* usage string for error messages */
    Tcl_Obj *argumentPtr;       /* the function arguments */
    Tcl_Obj *bodyPtr;           /* the function body */
    ItclArgList *argListPtr;    /* the parsed arguments */
    union {
        Tcl_CmdProc *argCmd;    /* (argc,argv) C implementation */
        Tcl_ObjCmdProc *objCmd; /* (objc,objv) C implementation */
    } cfunc;
    ClientData clientData;      /* client data for C implementations */
} ItclMemberCode;

/*
 *  Flag bits for ItclMemberCode:
 */
#define ITCL_IMPLEMENT_NONE    0x001  /* no implementation */
#define ITCL_IMPLEMENT_TCL     0x002  /* Tcl implementation */
#define ITCL_IMPLEMENT_ARGCMD  0x004  /* (argc,argv) C implementation */
#define ITCL_IMPLEMENT_OBJCMD  0x008  /* (objc,objv) C implementation */
#define ITCL_IMPLEMENT_C       0x00c  /* either kind of C implementation */

#define Itcl_IsMemberCodeImplemented(mcode) \
    (((mcode)->flags & ITCL_IMPLEMENT_NONE) == 0)

/*
 *  Flag bits for ItclMember:
 */
#define ITCL_CONSTRUCTOR       0x010  /* non-zero => is a constructor */
#define ITCL_DESTRUCTOR        0x020  /* non-zero => is a destructor */
#define ITCL_COMMON            0x040  /* non-zero => is a "proc" */
#define ITCL_ARG_SPEC          0x080  /* non-zero => has an argument spec */
#define ITCL_BODY_SPEC         0x100  /* non-zero => has an body spec */
#define ITCL_THIS_VAR          0x200  /* non-zero => built-in "this" variable */
#define ITCL_CONINIT           0x400  /* non-zero => is a constructor
                                       * init code */
#define ITCL_BUILTIN           0x800  /* non-zero => built-in method */
#define ITCL_OPTION_READONLY   0x1000 /* non-zero => readonly */
#define ITCL_COMPONENT         0x2000 /* non-zero => component */
#define ITCL_OPTIONS_VAR       0x400  /* non-zero => built-in "itcl_options"
                                       * variable */

/*
 *  Instance components.
 */
struct ItclVariable;
typedef struct ItclComponent {
    Tcl_Obj *namePtr;           /* member name */
    struct ItclVariable *ivPtr; /* variable for this component */
    int flags;
} ItclComponent;

#define ITCL_COMPONENT_INHERIT	0x01

typedef struct ItclDelegatedFunction {
    Tcl_Obj *namePtr;
    ItclComponent *icPtr;
    Tcl_Obj *asPtr;
    Tcl_Obj *usingPtr;
    Tcl_HashTable exceptions;
    int flags;
} ItclDelegatedFunction;

/*
 *  Representation of member functions in an [incr Tcl] class.
 */
typedef struct ItclMemberFunc {
    Tcl_Obj* namePtr;           /* member name */
    Tcl_Obj* fullNamePtr;       /* member name with "class::" qualifier */
    ItclClass* iclsPtr;         /* class containing this member */
    int protection;             /* protection level */
    int flags;                  /* flags describing member (see above) */
    ItclMemberCode *codePtr;    /* code associated with member */
    Tcl_Command accessCmd;       /* Tcl command installed for this function */
    int argcount;                /* number of args in arglist */
    int maxargcount;             /* max number of args in arglist */
    Tcl_Obj *usagePtr;          /* usage string for error messages */
    Tcl_Obj *argumentPtr;       /* the function arguments */
    Tcl_Obj *origArgsPtr;       /* the argument string of the original definition */
    Tcl_Obj *bodyPtr;           /* the function body */
    ItclArgList *argListPtr;    /* the parsed arguments */
    ItclClass *declaringClassPtr; /* the class which declared the method/proc */
    ClientData tmPtr;           /* TclOO methodPtr */
    ItclDelegatedFunction *idmPtr;
                                /* if the function is delegated != NULL */
} ItclMemberFunc;

/*
 *  Instance variables.
 */
typedef struct ItclVariable {
    Tcl_Obj *namePtr;           /* member name */
    Tcl_Obj *fullNamePtr;       /* member name with "class::" qualifier */
    ItclClass *iclsPtr;         /* class containing this member */
    int protection;             /* protection level */
    int flags;                  /* flags describing member (see below) */
    ItclMemberCode *codePtr;    /* code associated with member */
    Tcl_Obj *init;              /* initial value */
} ItclVariable;


struct ItclOption;

typedef struct ItclDelegatedOption {
    Tcl_Obj *namePtr;
    Tcl_Obj *resourceNamePtr;
    Tcl_Obj *classNamePtr;
    struct ItclOption *ioptPtr;  /* the option name or null for "*" */
    ItclComponent *icPtr;        /* the component where the delegation goes
                                  * to */
    Tcl_Obj *asPtr;
    Tcl_HashTable exceptions;    /* exceptions from delegation */
} ItclDelegatedOption;

/*
 *  Instance options.
 */
typedef struct ItclOption {
                                /* within a class hierarchy there must be only
				 * one option with the same name !! */
    Tcl_Obj *namePtr;           /* member name */
    Tcl_Obj *fullNamePtr;       /* member name with "class::" qualifier */
    Tcl_Obj *resourceNamePtr;
    Tcl_Obj *classNamePtr;
    ItclClass *iclsPtr;         /* class containing this member */
    int protection;             /* protection level */
    int flags;                  /* flags describing member (see below) */
    ItclMemberCode *codePtr;    /* code associated with member */
    Tcl_Obj *defaultValuePtr;   /* initial value */
    Tcl_Obj *cgetMethodPtr;
    Tcl_Obj *cgetMethodVarPtr;
    Tcl_Obj *configureMethodPtr;
    Tcl_Obj *configureMethodVarPtr;
    Tcl_Obj *validateMethodPtr;
    Tcl_Obj *validateMethodVarPtr;
    ItclDelegatedOption *idoPtr;
                                /* if the option is delegated != NULL */
} ItclOption;

/*
 *  Instance methodvariables.
 */
typedef struct ItclMethodVariable {
    Tcl_Obj *namePtr;           /* member name */
    Tcl_Obj *fullNamePtr;       /* member name with "class::" qualifier */
    ItclClass *iclsPtr;         /* class containing this member */
    int protection;             /* protection level */
    int flags;                  /* flags describing member (see below) */
    Tcl_Obj *defaultValuePtr;
    Tcl_Obj *callbackPtr;
} ItclMethodVariable;

typedef struct IctlVarTraceInfo {
    int flags;
    ItclVariable* ivPtr;
    ItclClass *iclsPtr;
    ItclObject *ioPtr;
} IctlVarTraceInfo;

#define ITCL_TRACE_CLASS		0x01
#define ITCL_TRACE_OBJECT		0x02

/*
 *  Instance variable lookup entry.
 */
typedef struct ItclVarLookup {
    ItclVariable* ivPtr;      /* variable definition */
    int usage;                /* number of uses for this record */
    int accessible;           /* non-zero => accessible from class with
                               * this lookup record in its resolveVars */
    char *leastQualName;      /* simplist name for this variable, with
                               * the fewest qualifiers.  This string is
                               * taken from the resolveVars table, so
                               * it shouldn't be freed. */
} ItclVarLookup;

typedef struct ItclCallContext {
    int objectFlags;
    int classFlags;
    Tcl_Namespace *nsPtr;
    ItclObject *ioPtr;
    ItclClass *iclsPtr;
    ItclMemberFunc *imPtr;
    int refCount;
} ItclCallContext;

#ifdef ITCL_DEBUG
MODULE_SCOPE int _itcl_debug_level;
MODULE_SCOPE void ItclShowArgs(int level, const char *str, int objc,
	Tcl_Obj * const* objv);
#else
#define ItclShowArgs(a,b,c,d)
#endif

MODULE_SCOPE Tcl_ObjCmdProc ItclCallCCommand;
MODULE_SCOPE Tcl_ObjCmdProc ItclObjectUnknownCommand;
MODULE_SCOPE int ItclCheckCallProc(ClientData clientData, Tcl_Interp *interp,
        Tcl_ObjectContext contextPtr, Tcl_CallFrame *framePtr, int *isFinished);

MODULE_SCOPE ItclFoundation *ItclGetFoundation(Tcl_Interp *interp);
MODULE_SCOPE Tcl_ObjCmdProc ItclClassCommandDispatcher;
MODULE_SCOPE Tcl_Command Itcl_CmdAliasProc(Tcl_Interp *interp,
        Tcl_Namespace *nsPtr, CONST char *cmdName, ClientData clientData);
MODULE_SCOPE Tcl_Var Itcl_VarAliasProc(Tcl_Interp *interp,
        Tcl_Namespace *nsPtr, CONST char *VarName, ClientData clientData);
MODULE_SCOPE int ItclIsClass(Tcl_Interp *interp, Tcl_Command cmd);
MODULE_SCOPE int ItclCheckCallMethod(ClientData clientData, Tcl_Interp *interp,
        Tcl_ObjectContext contextPtr, Tcl_CallFrame *framePtr, int *isFinished);
MODULE_SCOPE int ItclAfterCallMethod(ClientData clientData, Tcl_Interp *interp,
        Tcl_ObjectContext contextPtr, Tcl_Namespace *nsPtr, int result);
MODULE_SCOPE void ItclReportObjectUsage(Tcl_Interp *interp,
        ItclObject *contextIoPtr, Tcl_Namespace *callerNsPtr,
	Tcl_Namespace *contextNsPtr);
MODULE_SCOPE void ItclGetInfoUsage(Tcl_Interp *interp, Tcl_Obj *objPtr);
MODULE_SCOPE int ItclMapMethodNameProc(Tcl_Interp *interp, Tcl_Object oPtr,
        Tcl_Class *startClsPtr, Tcl_Obj *methodObj);
MODULE_SCOPE int ItclCreateArgList(Tcl_Interp *interp, const char *str,
        int *argcPtr, int *maxArgcPtr, Tcl_Obj **usagePtr,
	ItclArgList **arglistPtrPtr, ItclMemberFunc *imPtr,
	const char *commandName);
MODULE_SCOPE int ItclObjectCmd(ClientData clientData, Tcl_Interp *interp,
        Tcl_Object oPtr, Tcl_Class clsPtr, int objc, Tcl_Obj *const *objv);
MODULE_SCOPE int ItclCreateObject (Tcl_Interp *interp, CONST char* name,
        ItclClass *iclsPtr, int objc, Tcl_Obj *CONST objv[]);
MODULE_SCOPE void ItclDeleteObjectVariablesNamespace(Tcl_Interp *interp,
        ItclObject *ioPtr);
MODULE_SCOPE void ItclDeleteClassVariablesNamespace(Tcl_Interp *interp,
        ItclClass *iclsPtr);
MODULE_SCOPE int ItclInfoInit(Tcl_Interp *interp);
MODULE_SCOPE char * ItclTraceUnsetVar(ClientData clientData, Tcl_Interp *interp,
	const char *name1, const char *name2, int flags);

struct Tcl_ResolvedVarInfo;
MODULE_SCOPE int Itcl_ClassCmdResolver(Tcl_Interp *interp, CONST char* name,
	Tcl_Namespace *nsPtr, int flags, Tcl_Command *rPtr);
MODULE_SCOPE int Itcl_ClassVarResolver(Tcl_Interp *interp, CONST char* name,
        Tcl_Namespace *nsPtr, int flags, Tcl_Var *rPtr);
MODULE_SCOPE int Itcl_ClassCompiledVarResolver(Tcl_Interp *interp,
        CONST char* name, int length, Tcl_Namespace *nsPtr,
        struct Tcl_ResolvedVarInfo **rPtr);
MODULE_SCOPE int ItclSetParserResolver(Tcl_Namespace *nsPtr);
MODULE_SCOPE void ItclProcErrorProc(Tcl_Interp *interp, Tcl_Obj *procNameObj);
MODULE_SCOPE int ItclClassBaseCmd(ClientData clientData, Tcl_Interp *interp,
	int flags, int objc, Tcl_Obj *CONST objv[], ItclClass **iclsPtrPtr);
MODULE_SCOPE int Itcl_BiHullInstallCmd (ClientData clientData,
        Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
MODULE_SCOPE int Itcl_CreateOption (Tcl_Interp *interp, ItclClass *iclsPtr,
	ItclOption *ioptPtr);
MODULE_SCOPE int Itcl_CreateMethodVariable (Tcl_Interp *interp,
        ItclClass *iclsPtr, Tcl_Obj *name, Tcl_Obj *defaultPtr,
	Tcl_Obj *callbackPtr, ItclMethodVariable **imvPtr);
MODULE_SCOPE int ItclInitObjectOptions(Tcl_Interp *interp, ItclObject *ioPtr,
        ItclClass *iclsPtr, const char *name);
MODULE_SCOPE int HullAndOptionsInstall(Tcl_Interp *interp, ItclObject *ioPtr,
        ItclClass *iclsPtr, int objc, Tcl_Obj * const objv[],
	int *newObjc, Tcl_Obj **newObjv);
MODULE_SCOPE int DelegationInstall(Tcl_Interp *interp, ItclObject *ioPtr,
        ItclClass *iclsPtr);
MODULE_SCOPE const char* ItclGetInstanceVar(Tcl_Interp *interp,
        const char *name, const char *name2, ItclObject *contextIoPtr,
	ItclClass *contextIclsPtr);
MODULE_SCOPE const char* ItclSetInstanceVar(Tcl_Interp *interp,
        const char *name, const char *name2, const char *value,
	ItclObject *contextIoPtr, ItclClass *contextIclsPtr);
MODULE_SCOPE Tcl_Obj * ItclCapitalize(const char *str);
MODULE_SCOPE int ItclExtendedConfigure(ClientData clientData,
        Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
MODULE_SCOPE int ItclExtendedCget(ClientData clientData, Tcl_Interp *interp,
        int objc, Tcl_Obj *const objv[]);
MODULE_SCOPE int ItclCreateMethod(Tcl_Interp* interp, ItclClass *iclsPtr,
	Tcl_Obj *namePtr, const char* arglist, const char* body,
        ItclMemberFunc **imPtrPtr);
MODULE_SCOPE int ItclCreateComponent(Tcl_Interp *interp, ItclClass *iclsPtr,
        Tcl_Obj *componentPtr, ItclComponent **icPtrPtr);
MODULE_SCOPE int Itcl_WidgetParseInit(Tcl_Interp *interp,
        ItclObjectInfo *infoPtr);
MODULE_SCOPE int Itcl_WidgetBiInit(Tcl_Interp *interp);
MODULE_SCOPE int ItclWidgetInfoInit(Tcl_Interp *interp);
MODULE_SCOPE void ItclDeleteObjectMetadata(ClientData clientData);
MODULE_SCOPE void ItclDeleteClassMetadata(ClientData clientData);
MODULE_SCOPE void ItclDeleteArgList(ItclArgList *arglistPtr);
MODULE_SCOPE int Itcl_ClassOptionCmd(ClientData clientData, Tcl_Interp *interp,
        int objc, Tcl_Obj *CONST objv[]);
MODULE_SCOPE int DelegatedOptionsInstall(Tcl_Interp *interp,
        ItclClass *iclsPtr);
MODULE_SCOPE int ItclInitObjectOptions(Tcl_Interp *interp, ItclObject *ioPtr,
         ItclClass *iclsPtr, const char *name);
MODULE_SCOPE int Itcl_HandleDelegateOptionCmd(Tcl_Interp *interp,
        ItclObject *ioPtr, ItclClass *iclsPtr, ItclDelegatedOption **idoPtrPtr,
        int objc, Tcl_Obj *CONST objv[]);
MODULE_SCOPE int Itcl_HandleDelegateMethodCmd(Tcl_Interp *interp,
        ItclObject *ioPtr, ItclClass *iclsPtr,
	ItclDelegatedFunction **idmPtrPtr, int objc, Tcl_Obj *CONST objv[]);
MODULE_SCOPE int DelegateFunction(Tcl_Interp *interp, ItclObject *ioPtr,
        ItclClass *iclsPtr, Tcl_Obj *componentNamePtr,
        ItclDelegatedFunction *idmPtr);
MODULE_SCOPE int ItclInitObjectMethodVariables(Tcl_Interp *interp,
        ItclObject *ioPtr, ItclClass *iclsPtr, const char *name);
MODULE_SCOPE int InitTclOOFunctionPointers(Tcl_Interp *interp);
MODULE_SCOPE ItclOption* ItclNewOption(Tcl_Interp *interp, ItclObject *ioPtr,
        ItclClass *iclsPtr, Tcl_Obj *namePtr, const char *resourceName,
        const char *className, char *init, ItclMemberCode *mCodePtr);
MODULE_SCOPE int ItclParseOption(ItclObjectInfo *infoPtr, Tcl_Interp *interp,
        int objc, Tcl_Obj *CONST objv[], ItclOption **ioptPtrPtr);

#include "itclng2TclOO.h"

/*
 * Include all the private API, generated from tclOO.decls.
 */

#include "itclngIntDecls.h"
