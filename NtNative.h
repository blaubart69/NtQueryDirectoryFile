#pragma once

#define STATUS_NO_MORE_FILES             ((NTSTATUS)0x80000006L)

typedef _Return_type_success_(return >= 0) LONG NTSTATUS;

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

	typedef enum _FILE_INFORMATION_CLASS {
		FileDirectoryInformation = 1,
		FileFullDirectoryInformation,   // 2
		FileBothDirectoryInformation,   // 3
		FileBasicInformation,           // 4
		FileStandardInformation,        // 5
		FileInternalInformation,        // 6
		FileEaInformation,              // 7
		FileAccessInformation,          // 8
		FileNameInformation,            // 9
		FileRenameInformation,          // 10
		FileLinkInformation,            // 11
		FileNamesInformation,           // 12
		FileDispositionInformation,     // 13
		FilePositionInformation,        // 14
		FileFullEaInformation,          // 15
		FileModeInformation,            // 16
		FileAlignmentInformation,       // 17
		FileAllInformation,             // 18
		FileAllocationInformation,      // 19
		FileEndOfFileInformation,       // 20
		FileAlternateNameInformation,   // 21
		FileStreamInformation,          // 22
		FilePipeInformation,            // 23
		FilePipeLocalInformation,       // 24
		FilePipeRemoteInformation,      // 25
		FileMailslotQueryInformation,   // 26
		FileMailslotSetInformation,     // 27
		FileCompressionInformation,     // 28
		FileObjectIdInformation,        // 29
		FileCompletionInformation,      // 30
		FileMoveClusterInformation,     // 31
		FileQuotaInformation,           // 32
		FileReparsePointInformation,    // 33
		FileNetworkOpenInformation,     // 34
		FileAttributeTagInformation,    // 35
		FileTrackingInformation,        // 36
		FileIdBothDirectoryInformation, // 37
		FileIdFullDirectoryInformation, // 38
		FileValidDataLengthInformation, // 39
		FileShortNameInformation,       // 40
		FileIoCompletionNotificationInformation, // 41
		FileIoStatusBlockRangeInformation,       // 42
		FileIoPriorityHintInformation,           // 43
		FileSfioReserveInformation,              // 44
		FileSfioVolumeInformation,               // 45
		FileHardLinkInformation,                 // 46
		FileProcessIdsUsingFileInformation,      // 47
		FileNormalizedNameInformation,           // 48
		FileNetworkPhysicalNameInformation,      // 49
		FileIdGlobalTxDirectoryInformation,      // 50
		FileIsRemoteDeviceInformation,           // 51
		FileUnusedInformation,                   // 52
		FileNumaNodeInformation,                 // 53
		FileStandardLinkInformation,             // 54
		FileRemoteProtocolInformation,           // 55

												 //
												 //  These are special versions of these operations (defined earlier)
												 //  which can be used by kernel mode drivers only to bypass security
												 //  access checks for Rename and HardLink operations.  These operations
												 //  are only recognized by the IOManager, a file system should never
												 //  receive these.
												 //

												 FileRenameInformationBypassAccessCheck,  // 56
												 FileLinkInformationBypassAccessCheck,    // 57

																						  //
																						  // End of special information classes reserved for IOManager.
																						  //

																						  FileVolumeNameInformation,               // 58
																						  FileIdInformation,                       // 59
																						  FileIdExtdDirectoryInformation,          // 60
																						  FileReplaceCompletionInformation,        // 61
																						  FileHardLinkFullIdInformation,           // 62
																						  FileIdExtdBothDirectoryInformation,      // 63
																						  FileDispositionInformationEx,            // 64
																						  FileRenameInformationEx,                 // 65
																						  FileRenameInformationExBypassAccessCheck, // 66
																						  FileDesiredStorageClassInformation,      // 67
																						  FileStatInformation,                     // 68
																						  FileMemoryPartitionInformation,          // 69
																						  FileStatLxInformation,                   // 70
																						  FileCaseSensitiveInformation,            // 71
																						  FileMaximumInformation
	} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;


	typedef struct _IO_STATUS_BLOCK {
		union {
			NTSTATUS Status;
			PVOID Pointer;
		} DUMMYUNIONNAME;

		ULONG_PTR Information;
	} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

	typedef struct FILE_DIRECTORY_INFORMATION {
		unsigned long NextEntryOffset;
		unsigned long FileIndex;
		__int64 CreationTime;
		__int64 LastAccessTime;
		__int64 LastWriteTime;
		__int64 ChangeTime;
		__int64 EndOfFile;
		__int64 AllocationSize;
		unsigned long FileAttributes;
		unsigned long FileNameLength;
		wchar_t FileName[1];
	} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

	//
	// Define an Asynchronous Procedure Call from I/O viewpoint
	//

	typedef
		VOID
		(NTAPI *PIO_APC_ROUTINE) (
			_In_ PVOID ApcContext,
			_In_ PIO_STATUS_BLOCK IoStatusBlock,
			_In_ ULONG Reserved
			);

	typedef struct _UNICODE_STRING {
		USHORT Length;
		USHORT MaximumLength;
#ifdef MIDL_PASS
		[size_is(MaximumLength / 2), length_is((Length) / 2)] USHORT * Buffer;
#else // MIDL_PASS
		_Field_size_bytes_part_opt_(MaximumLength, Length) PWCH   Buffer;
#endif // MIDL_PASS
	} UNICODE_STRING;
	typedef UNICODE_STRING *PUNICODE_STRING;

	__kernel_entry NTSYSCALLAPI
		NTSTATUS
		NTAPI
		NtQueryDirectoryFile(
			_In_ HANDLE FileHandle,
			_In_opt_ HANDLE Event,
			_In_opt_ PIO_APC_ROUTINE ApcRoutine,
			_In_opt_ PVOID ApcContext,
			_Out_ PIO_STATUS_BLOCK IoStatusBlock,
			_Out_writes_bytes_(Length) PVOID FileInformation,
			_In_ ULONG Length,
			_In_ FILE_INFORMATION_CLASS FileInformationClass,
			_In_ BOOLEAN ReturnSingleEntry,
			_In_opt_ PUNICODE_STRING FileName,
			_In_ BOOLEAN RestartScan
		);


	

