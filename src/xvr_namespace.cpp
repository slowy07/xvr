// ===========================================================
// XVR Namespace Infrastructure - Implementation
// ===========================================================
// This module provides namespace support for the XVR compiler.
// It allows registering namespaces and their members.
//
// For stage0: Start with built-in namespaces (std, math) and make it
// easy to add more namespaces in the bootstrap compiler.
// ===========================================================

#include "xvr_namespace.h"
#include "xvr_memory.h"
#include <string.h>
#include <stdlib.h>

// ===========================================================
// Static Data
// ===========================================================

#define XVR_MAX_NAMESPACES 32
static Xvr_Namespace namespaces[XVR_MAX_NAMESPACES];
static int namespace_count = 0;

// ===========================================================
// Internal Helpers
// ===========================================================

static Xvr_Namespace* find_namespace(const char* name) {
    for (int i = 0; i < namespace_count; i++) {
        if (strcmp(namespaces[i].name, name) == 0) {
            return &namespaces[i];
        }
    }
    return NULL;
}

// ===========================================================
// Public API
// ===========================================================

void Xvr_NamespaceInit(void) {
    namespace_count = 0;
    memset(namespaces, 0, sizeof(namespaces));
}

void Xvr_NamespaceCleanup(void) {
    for (int i = 0; i < namespace_count; i++) {
        Xvr_Namespace* ns = &namespaces[i];
        if (ns->members) {
            XVR_FREE(Xvr_NamespaceMember, ns->members);
            ns->members = NULL;
        }
        ns->member_count = 0;
        ns->member_capacity = 0;
    }
    namespace_count = 0;
}

bool Xvr_NamespaceRegister(const char* name) {
    if (namespace_count >= XVR_MAX_NAMESPACES) {
        return false;
    }
    
    if (Xvr_NamespaceFind(name) != NULL) {
        return false;  // Already exists
    }
    
    Xvr_Namespace* ns = &namespaces[namespace_count++];
    ns->name = name;  // Assumes name is a string constant
    ns->members = NULL;
    ns->member_count = 0;
    ns->member_capacity = 0;
    
    return true;
}

Xvr_Namespace* Xvr_NamespaceFind(const char* name) {
    return find_namespace(name);
}

bool Xvr_NamespaceAddMember(const char* ns_name, const char* member_name,
                           Xvr_NamespaceMemberType type, void* data) {
    Xvr_Namespace* ns = find_namespace(ns_name);
    if (!ns) {
        return false;
    }
    
    // Check if member already exists
    for (int i = 0; i < ns->member_count; i++) {
        if (strcmp(ns->members[i].name, member_name) == 0) {
            return false;  // Already exists
        }
    }
    
    // Grow members array if needed
    if (ns->member_count >= ns->member_capacity) {
        int new_capacity = ns->member_capacity == 0 ? 4 : ns->member_capacity * 2;
        Xvr_NamespaceMember* new_members = XVR_GROW_ARRAY(
            Xvr_NamespaceMember, ns->members, ns->member_capacity, new_capacity);
        if (!new_members) {
            return false;
        }
        ns->members = new_members;
        ns->member_capacity = new_capacity;
    }
    
    // Add the member
    Xvr_NamespaceMember* member = &ns->members[ns->member_count++];
    member->name = member_name;  // Assumes name is a string constant
    member->type = type;
    member->data = data;
    
    return true;
}

Xvr_NamespaceMember* Xvr_NamespaceFindMember(const char* ns_name,
                                               const char* member_name) {
    Xvr_Namespace* ns = find_namespace(ns_name);
    if (!ns) {
        return NULL;
    }
    
    for (int i = 0; i < ns->member_count; i++) {
        if (strcmp(ns->members[i].name, member_name) == 0) {
            return &ns->members[i];
        }
    }
    return NULL;
}

bool Xvr_NamespaceIsValidAccess(const char* ns_name, const char* member_name) {
    return Xvr_NamespaceFindMember(ns_name, member_name) != NULL;
}

// ===========================================================
// Built-in Namespaces
// ===========================================================

void Xvr_NamespaceRegisterBuiltins(void) {
    // Register std namespace
    Xvr_NamespaceRegister("std");
    Xvr_NamespaceAddMember("std", "print", XVR_NS_MEMBER_FUNCTION, (void*)"print");
    Xvr_NamespaceAddMember("std", "println", XVR_NS_MEMBER_FUNCTION, (void*)"println");
    
    // Register math namespace
    Xvr_NamespaceRegister("math");
    Xvr_NamespaceAddMember("math", "sqrt", XVR_NS_MEMBER_FUNCTION, (void*)"sqrt");
    Xvr_NamespaceAddMember("math", "pow", XVR_NS_MEMBER_FUNCTION, (void*)"pow");
    Xvr_NamespaceAddMember("math", "sin", XVR_NS_MEMBER_FUNCTION, (void*)"sin");
    Xvr_NamespaceAddMember("math", "cos", XVR_NS_MEMBER_FUNCTION, (void*)"cos");
    Xvr_NamespaceAddMember("math", "abs", XVR_NS_MEMBER_FUNCTION, (void*)"abs");
    Xvr_NamespaceAddMember("math", "floor", XVR_NS_MEMBER_FUNCTION, (void*)"floor");
    Xvr_NamespaceAddMember("math", "ceil", XVR_NS_MEMBER_FUNCTION, (void*)"ceil");
    Xvr_NamespaceAddMember("math", "round", XVR_NS_MEMBER_FUNCTION, (void*)"round");
    Xvr_NamespaceAddMember("math", "log", XVR_NS_MEMBER_FUNCTION, (void*)"log");
    Xvr_NamespaceAddMember("math", "atan2", XVR_NS_MEMBER_FUNCTION, (void*)"atan2");
    Xvr_NamespaceAddMember("math", "fmod", XVR_NS_MEMBER_FUNCTION, (void*)"fmod");
    Xvr_NamespaceAddMember("math", "max", XVR_NS_MEMBER_FUNCTION, (void*)"max");
    Xvr_NamespaceAddMember("math", "min", XVR_NS_MEMBER_FUNCTION, (void*)"min");
}
