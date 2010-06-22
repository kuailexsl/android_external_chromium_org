// Copyright (c) 2006-2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/src/sandbox_nt_util.h"

#include "base/pe_image.h"
#include "sandbox/src/sandbox_factory.h"
#include "sandbox/src/target_services.h"

namespace sandbox {

// This is the list of all imported symbols from ntdll.dll.
SANDBOX_INTERCEPT NtExports g_nt = { NULL };

}

namespace {

#if defined(_WIN64)
void* AllocateNearTo(void* source, size_t size) {
  using sandbox::g_nt;

  // Start with 1 GB above the source.
  const unsigned int kOneGB = 0x40000000;
  void* base = reinterpret_cast<char*>(source) + kOneGB;
  SIZE_T actual_size = size;
  ULONG_PTR zero_bits = 0;  // Not the correct type if used.
  ULONG type = MEM_RESERVE;

  if (reinterpret_cast<SIZE_T>(source) > 0x7ff80000000) {
    // We are at the top of the address space. Let's try the highest available
    // address.
    base = NULL;
    type |= MEM_TOP_DOWN;
  }

  NTSTATUS ret;
  int attempts = 0;
  for (; attempts < 20; attempts++) {
    ret = g_nt.AllocateVirtualMemory(NtCurrentProcess, &base, zero_bits,
                                     &actual_size, type, PAGE_READWRITE);
    if (NT_SUCCESS(ret)) {
      if (base < source) {
        // We won't be able to patch this dll.
        VERIFY_SUCCESS(g_nt.FreeVirtualMemory(NtCurrentProcess, &base, &size,
                                              MEM_RELEASE));
        return NULL;
      }
      break;
    }

    // Try 100 MB higher.
    base = reinterpret_cast<char*>(base) + 100 * 0x100000;
  };

  if (attempts == 20)
    return NULL;

  ret = g_nt.AllocateVirtualMemory(NtCurrentProcess, &base, zero_bits,
                                   &actual_size, MEM_COMMIT, PAGE_READWRITE);

  if (!NT_SUCCESS(ret)) {
    VERIFY_SUCCESS(g_nt.FreeVirtualMemory(NtCurrentProcess, &base, &size,
                                          MEM_RELEASE));
    base = NULL;
  }

  return base;
}
#else  // defined(_WIN64).
void* AllocateNearTo(void* source, size_t size) {
  using sandbox::g_nt;
  UNREFERENCED_PARAMETER(source);
  void* base = 0;
  SIZE_T actual_size = size;
  ULONG_PTR zero_bits = 0;  // Not the correct type if used.
  NTSTATUS ret = g_nt.AllocateVirtualMemory(NtCurrentProcess, &base,
                                            zero_bits, &actual_size,
                                            MEM_COMMIT, PAGE_READWRITE);
  if (!NT_SUCCESS(ret))
    return NULL;
  return base;
}
#endif  // defined(_WIN64).

}  // namespace.

namespace sandbox {

// Handle for our private heap.
void* g_heap = NULL;

SANDBOX_INTERCEPT HANDLE g_shared_section;
SANDBOX_INTERCEPT size_t g_shared_IPC_size = 0;
SANDBOX_INTERCEPT size_t g_shared_policy_size = 0;

void* volatile g_shared_policy_memory = NULL;
void* volatile g_shared_IPC_memory = NULL;

// Both the IPC and the policy share a single region of memory in which the IPC
// memory is first and the policy memory is last.
bool MapGlobalMemory() {
  if (NULL == g_shared_IPC_memory) {
    void* memory = NULL;
    SIZE_T size = 0;
    // Map the entire shared section from the start.
    NTSTATUS ret = g_nt.MapViewOfSection(g_shared_section, NtCurrentProcess,
                                         &memory, 0, 0, NULL, &size, ViewUnmap,
                                         0, PAGE_READWRITE);

    if (!NT_SUCCESS(ret) || NULL == memory) {
      NOTREACHED_NT();
      return false;
    }

    if (NULL != _InterlockedCompareExchangePointer(&g_shared_IPC_memory,
                                                   memory, NULL)) {
        // Somebody beat us to the memory setup.
        ret = g_nt.UnmapViewOfSection(NtCurrentProcess, memory);
        VERIFY_SUCCESS(ret);
    }
    DCHECK_NT(g_shared_IPC_size > 0);
    g_shared_policy_memory = reinterpret_cast<char*>(g_shared_IPC_memory)
                             + g_shared_IPC_size;
  }
  DCHECK_NT(g_shared_policy_memory);
  DCHECK_NT(g_shared_policy_size > 0);
  return true;
}

void* GetGlobalIPCMemory() {
  if (!MapGlobalMemory())
    return NULL;
  return g_shared_IPC_memory;
}

void* GetGlobalPolicyMemory() {
  if (!MapGlobalMemory())
    return NULL;
  return g_shared_policy_memory;
}

bool InitHeap() {
  if (!g_heap) {
    // Create a new heap using default values for everything.
    void* heap = g_nt.RtlCreateHeap(HEAP_GROWABLE, NULL, 0, 0, NULL, NULL);
    if (!heap)
      return false;

    if (NULL != _InterlockedCompareExchangePointer(&g_heap, heap, NULL)) {
      // Somebody beat us to the memory setup.
      g_nt.RtlDestroyHeap(heap);
    }
  }
  return (g_heap) ? true : false;
}

// Physically reads or writes from memory to verify that (at this time), it is
// valid. Returns a dummy value.
int TouchMemory(void* buffer, size_t size_bytes, RequiredAccess intent) {
  const int kPageSize = 4096;
  int dummy = 0;
  char* start = reinterpret_cast<char*>(buffer);
  char* end = start + size_bytes - 1;

  if (WRITE == intent) {
    for (; start < end; start += kPageSize) {
      *start = 0;
    }
    *end = 0;
  } else {
    for (; start < end; start += kPageSize) {
      dummy += *start;
    }
    dummy += *end;
  }

  return dummy;
}

bool ValidParameter(void* buffer, size_t size, RequiredAccess intent) {
  DCHECK_NT(size);
  __try {
    TouchMemory(buffer, size, intent);
  } __except(EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
  return true;
}

NTSTATUS CopyData(void* destination, const void* source, size_t bytes) {
  NTSTATUS ret = STATUS_SUCCESS;
  __try {
    if (SandboxFactory::GetTargetServices()->GetState()->InitCalled()) {
      memcpy(destination, source, bytes);
    } else {
      const char* from = reinterpret_cast<const char*>(source);
      char* to = reinterpret_cast<char*>(destination);
      for (size_t i = 0; i < bytes; i++) {
        to[i] = from[i];
      }
    }
  } __except(EXCEPTION_EXECUTE_HANDLER) {
    ret = GetExceptionCode();
  }
  return ret;
}

// Hacky code... replace with AllocAndCopyObjectAttributes.
NTSTATUS AllocAndCopyName(const OBJECT_ATTRIBUTES* in_object,
                          wchar_t** out_name, uint32* attributes,
                          HANDLE* root) {
  if (!InitHeap())
    return STATUS_NO_MEMORY;

  DCHECK_NT(out_name);
  *out_name = NULL;
  NTSTATUS ret = STATUS_UNSUCCESSFUL;
  __try {
    do {
      if (in_object->RootDirectory != static_cast<HANDLE>(0) && !root)
        break;
      if (NULL == in_object->ObjectName)
        break;
      if (NULL == in_object->ObjectName->Buffer)
        break;

      size_t size = in_object->ObjectName->Length + sizeof(wchar_t);
      *out_name = new(NT_ALLOC) wchar_t[size/sizeof(wchar_t)];
      if (NULL == *out_name)
        break;

      ret = CopyData(*out_name, in_object->ObjectName->Buffer,
                     size - sizeof(wchar_t));
      if (!NT_SUCCESS(ret))
        break;

      (*out_name)[size / sizeof(wchar_t) - 1] = L'\0';

      if (attributes)
        *attributes = in_object->Attributes;

      if (root)
        *root = in_object->RootDirectory;
      ret = STATUS_SUCCESS;
    } while (false);
  } __except(EXCEPTION_EXECUTE_HANDLER) {
    ret = GetExceptionCode();
  }

  if (!NT_SUCCESS(ret) && *out_name) {
    operator delete(*out_name, NT_ALLOC);
    *out_name = NULL;
  }

  return ret;
}

NTSTATUS GetProcessId(HANDLE process, ULONG *process_id) {
  PROCESS_BASIC_INFORMATION proc_info;
  ULONG bytes_returned;

  NTSTATUS ret = g_nt.QueryInformationProcess(process, ProcessBasicInformation,
                                              &proc_info, sizeof(proc_info),
                                              &bytes_returned);
  if (!NT_SUCCESS(ret) || sizeof(proc_info) != bytes_returned)
    return ret;

  *process_id = proc_info.UniqueProcessId;
  return STATUS_SUCCESS;
}

bool IsSameProcess(HANDLE process) {
  if (NtCurrentProcess == process)
    return true;

  static ULONG s_process_id = 0;

  if (!s_process_id) {
    NTSTATUS ret = GetProcessId(NtCurrentProcess, &s_process_id);
    if (!NT_SUCCESS(ret))
      return false;
  }

  ULONG process_id;
  NTSTATUS ret = GetProcessId(process, &process_id);
  if (!NT_SUCCESS(ret))
    return false;

  return (process_id == s_process_id);
}

bool IsValidImageSection(HANDLE section, PVOID *base, PLARGE_INTEGER offset,
                         PSIZE_T view_size) {
  if (!section || !base || !view_size || offset)
    return false;

  HANDLE query_section;

  NTSTATUS ret = g_nt.DuplicateObject(NtCurrentProcess, section,
                                      NtCurrentProcess, &query_section,
                                      SECTION_QUERY, 0, 0);
  if (!NT_SUCCESS(ret))
    return false;

  SECTION_BASIC_INFORMATION basic_info;
  SIZE_T bytes_returned;
  ret = g_nt.QuerySection(query_section, SectionBasicInformation, &basic_info,
                          sizeof(basic_info), &bytes_returned);

  VERIFY_SUCCESS(g_nt.Close(query_section));

  if (!NT_SUCCESS(ret) || sizeof(basic_info) != bytes_returned)
    return false;

  if (!(basic_info.Attributes & SEC_IMAGE))
    return false;

  return true;
}

UNICODE_STRING* AnsiToUnicode(const char* string) {
  ANSI_STRING ansi_string;
  ansi_string.Length = static_cast<USHORT>(g_nt.strlen(string));
  ansi_string.MaximumLength = ansi_string.Length + 1;
  ansi_string.Buffer = const_cast<char*>(string);

  if (ansi_string.Length > ansi_string.MaximumLength)
    return NULL;

  size_t name_bytes = ansi_string.MaximumLength * sizeof(wchar_t) +
                      sizeof(UNICODE_STRING);

  UNICODE_STRING* out_string = reinterpret_cast<UNICODE_STRING*>(
                                   new(NT_ALLOC) char[name_bytes]);
  if (!out_string)
    return NULL;

  out_string->MaximumLength = ansi_string.MaximumLength *  sizeof(wchar_t);
  out_string->Buffer = reinterpret_cast<wchar_t*>(&out_string[1]);

  BOOLEAN alloc_destination = FALSE;
  NTSTATUS ret = g_nt.RtlAnsiStringToUnicodeString(out_string, &ansi_string,
                                                   alloc_destination);
  DCHECK_NT(STATUS_BUFFER_OVERFLOW != ret);
  if (!NT_SUCCESS(ret)) {
    operator delete(out_string, NT_ALLOC);
    return NULL;
  }

  return out_string;
}

UNICODE_STRING* GetImageInfoFromModule(HMODULE module, uint32* flags) {
  UNICODE_STRING* out_name = NULL;
  __try {
    do {
      *flags = 0;
      PEImage pe(module);

      if (!pe.VerifyMagic())
        break;
      *flags |= MODULE_IS_PE_IMAGE;

      PIMAGE_EXPORT_DIRECTORY exports = pe.GetExportDirectory();
      if (exports) {
        char* name = reinterpret_cast<char*>(pe.RVAToAddr(exports->Name));
        out_name = AnsiToUnicode(name);
      }

      PIMAGE_NT_HEADERS headers = pe.GetNTHeaders();
      if (headers) {
        if (headers->OptionalHeader.AddressOfEntryPoint)
          *flags |= MODULE_HAS_ENTRY_POINT;
        if (headers->OptionalHeader.SizeOfCode)
          *flags |= MODULE_HAS_CODE;
      }
    } while (false);
  } __except(EXCEPTION_EXECUTE_HANDLER) {
  }

  return out_name;
}

UNICODE_STRING* GetBackingFilePath(PVOID address) {
  // We'll start with something close to max_path charactes for the name.
  ULONG buffer_bytes = MAX_PATH * 2;

  for (;;) {
    MEMORY_SECTION_NAME* section_name = reinterpret_cast<MEMORY_SECTION_NAME*>(
        new(NT_ALLOC) char[buffer_bytes]);

    if (!section_name)
      return NULL;

    ULONG returned_bytes;
    NTSTATUS ret = g_nt.QueryVirtualMemory(NtCurrentProcess, address,
                                           MemorySectionName, section_name,
                                           buffer_bytes, &returned_bytes);

    if (STATUS_BUFFER_OVERFLOW == ret) {
      // Retry the call with the given buffer size.
      operator delete(section_name, NT_ALLOC);
      section_name = NULL;
      buffer_bytes = returned_bytes;
      continue;
    }
    if (!NT_SUCCESS(ret)) {
      operator delete(section_name, NT_ALLOC);
      return NULL;
    }

    return reinterpret_cast<UNICODE_STRING*>(section_name);
  }
}

UNICODE_STRING* ExtractModuleName(const UNICODE_STRING* module_path) {
  if ((!module_path) || (!module_path->Buffer))
    return NULL;

  wchar_t* sep = NULL;
  int start_pos = module_path->Length / sizeof(wchar_t) - 1;
  int ix = start_pos;

  for (; ix >= 0; --ix) {
    if (module_path->Buffer[ix] == L'\\') {
      sep = &module_path->Buffer[ix];
      break;
    }
  }

  // Ends with path separator. Not a valid module name.
  if ((ix == start_pos) && sep)
    return NULL;

  // No path separator found. Use the entire name.
  if (!sep) {
    sep = &module_path->Buffer[-1];
  }

  // Add one to the size so we can null terminate the string.
  size_t size_bytes = (start_pos - ix + 1) * sizeof(wchar_t);

  // Based on the code above, size_bytes should always be small enough
  // to make the static_cast below safe.
  DCHECK_NT(kuint16max > size_bytes);
  char* str_buffer = new(NT_ALLOC) char[size_bytes + sizeof(UNICODE_STRING)];
  if (!str_buffer)
    return NULL;

  UNICODE_STRING* out_string = reinterpret_cast<UNICODE_STRING*>(str_buffer);
  out_string->Buffer = reinterpret_cast<wchar_t*>(&out_string[1]);
  out_string->Length = static_cast<USHORT>(size_bytes - sizeof(wchar_t));
  out_string->MaximumLength = static_cast<USHORT>(size_bytes);

  NTSTATUS ret = CopyData(out_string->Buffer, &sep[1], out_string->Length);
  if (!NT_SUCCESS(ret)) {
    operator delete(out_string, NT_ALLOC);
    return NULL;
  }

  out_string->Buffer[out_string->Length / sizeof(wchar_t)] = L'\0';
  return out_string;
}

NTSTATUS AutoProtectMemory::ChangeProtection(void* address, size_t bytes,
                                             ULONG protect) {
  DCHECK_NT(!changed_);
  SIZE_T new_bytes = bytes;
  NTSTATUS ret = g_nt.ProtectVirtualMemory(NtCurrentProcess, &address,
                                           &new_bytes, protect, &old_protect_);
  if (NT_SUCCESS(ret)) {
    changed_ = true;
    address_ = address;
    bytes_ = bytes;
  }

  return ret;
}

NTSTATUS AutoProtectMemory::RevertProtection() {
  if (!changed_)
    return STATUS_SUCCESS;

  DCHECK_NT(address_);
  DCHECK_NT(bytes_);

  SIZE_T new_bytes = bytes_;
  NTSTATUS ret = g_nt.ProtectVirtualMemory(NtCurrentProcess, &address_,
                                           &new_bytes, old_protect_,
                                           &old_protect_);
  DCHECK_NT(NT_SUCCESS(ret));

  changed_ = false;
  address_ = NULL;
  bytes_ = 0;
  old_protect_ = 0;

  return ret;
}

bool IsSupportedRenameCall(FILE_RENAME_INFORMATION* file_info, DWORD length,
                           uint32 file_info_class) {
  if (FileRenameInformation != file_info_class)
    return false;

  if (length < sizeof(FILE_RENAME_INFORMATION))
    return false;

  // We don't support a root directory.
  if (file_info->RootDirectory)
    return false;

  // Check if it starts with \\??\\. We don't support relative paths.
  if (file_info->FileNameLength < 4 || file_info->FileNameLength > kuint16max)
    return false;

  if (file_info->FileName[0] != L'\\' ||
      file_info->FileName[1] != L'?' ||
      file_info->FileName[2] != L'?' ||
      file_info->FileName[3] != L'\\')
    return false;

  return true;
}

}  // namespace sandbox

void* operator new(size_t size, sandbox::AllocationType type,
                   void* near_to) {
  using namespace sandbox;

  if (NT_ALLOC == type) {
    if (!InitHeap())
      return false;

    // Use default flags for the allocation.
    return g_nt.RtlAllocateHeap(sandbox::g_heap, 0, size);
  } else if (NT_PAGE == type) {
    return AllocateNearTo(near_to, size);
  }
  NOTREACHED_NT();
  return NULL;
}

void operator delete(void* memory, sandbox::AllocationType type) {
  using namespace sandbox;

  if (NT_ALLOC == type) {
    // Use default flags.
    VERIFY(g_nt.RtlFreeHeap(sandbox::g_heap, 0, memory));
  } else if (NT_PAGE == type) {
    void* base = memory;
    SIZE_T size = 0;
    VERIFY_SUCCESS(g_nt.FreeVirtualMemory(NtCurrentProcess, &base, &size,
                                          MEM_RELEASE));
  } else {
    NOTREACHED_NT();
  }
}

void operator delete(void* memory, sandbox::AllocationType type,
                     void* near_to) {
  UNREFERENCED_PARAMETER(near_to);
  operator delete(memory, type);
}

void* __cdecl operator new(size_t size, void* buffer,
                           sandbox::AllocationType type) {
  UNREFERENCED_PARAMETER(size);
  UNREFERENCED_PARAMETER(type);
  return buffer;
}

void __cdecl operator delete(void* memory, void* buffer,
                             sandbox::AllocationType type) {
  UNREFERENCED_PARAMETER(memory);
  UNREFERENCED_PARAMETER(buffer);
  UNREFERENCED_PARAMETER(type);
}
