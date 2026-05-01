// ============================================================
// XVR Namespace Infrastructure
// ============================================================
// This module provides namespace support for the XVR compiler.
// It allows registering namespaces and their members (functions, constants, etc.)
// 
// For stage0: Start with built-in namespaces (std, math) and make it
// easy to add more namespaces in the bootstrap compiler.
// ============================================================

#ifndef XVR_NAMESPACE_H
#define XVR_NAMESPACE_H

#include "xvr_common.h"
#include "xvr_literal.h"
#include "xvr_lexer.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// Types
// ============================================================

typedef enum {
    XVR_NS_MEMBER_FUNCTION,
    XVR_NS_MEMBER_VARIABLE,
    XVR_NS_MEMBER_CONSTANT,
} Xvr_NamespaceMemberType;

typedef struct {
    const char* name;
    Xvr_NamespaceMemberType type;
    void* data;  // Function pointer, variable, etc.
} Xvr_NamespaceMember;

typedef struct {
    const char* name;
    Xvr_NamespaceMember* members;
    int member_count;
    int member_capacity;
} Xvr_Namespace;

// ============================================================
// Namespace Registry
// ============================================================

// Initialize namespace system
void Xvr_NamespaceInit(void);

// Cleanup namespace system  
void Xvr_NamespaceCleanup(void);

// Register a new namespace (returns false if already exists)
bool Xvr_NamespaceRegister(const char* name);

// Find a namespace by name (returns NULL if not found)
Xvr_Namespace* Xvr_NamespaceFind(const char* name);

// Add a member to a namespace
bool Xvr_NamespaceAddMember(const char* ns_name, const char* member_name,
                           Xvr_NamespaceMemberType type, void* data);

// Find a member in a namespace
Xvr_NamespaceMember* Xvr_NamespaceFindMember(const char* ns_name,
                                               const char* member_name);

// ============================================================
// Built-in Namespaces
// ============================================================

// Register all built-in namespaces (std, math, etc.)
void Xvr_NamespaceRegisterBuiltins(void);

// Check if a namespace::member is valid
bool Xvr_NamespaceIsValidAccess(const char* ns_name, const char* member_name);

#ifdef __cplusplus
}
#endif

#endif // XVR_NAMESPACE_H
