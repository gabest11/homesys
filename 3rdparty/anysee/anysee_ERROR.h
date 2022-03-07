#ifndef		__ANYSEE_ERROR_H__
#define		__ANYSEE_ERROR_H__

// ---------------------------------------------------------------
#define		ERROR_DWORD_VALUE			(DWORD)0xFFFFFFFF	// DWORD ｿﾀｷｪ

#define		NO_ERR		0
#define		ERROR_NO	NO_ERR
#define		ERR_NO		NO_ERR


#define		ERR_THREAD_CREATE						-2700	// ｾｲｷｹｵ・ｻｼｺ ｽﾇﾆﾐ
#define		ERR_THREAD_UNKNOWN_ID					-2701	// ｸ｣ｴﾂ ｾｲｷｹｵ・ID
#define		ERR_EVENT_CREATE						-2702	// ﾀﾌｺ･ﾆｮ ｻｼｺ ｽﾇﾆﾐ
#define		ERR_LINKED_EVENT						-2703	// ﾀﾌｹﾌ ｿｬｰ盞ﾈ ﾀﾌｺ･ﾆｮｰ｡ ﾀﾖﾀｽ.
#define		ERR_THREAD_FAILED_TO_CREATE				ERR_THREAD_CREATE
#define		ERR_THREAD_FAILED_TO_REFERENCE_OBJECT_HANDLE	-2704	// ｾｲｷｹｵ・ｰｴﾃｼ ﾇﾚｵ・ﾂ・ｶ ｽﾇﾆﾐ
#define		ERR_THREAD_RUNNING						-2705	// ﾀﾌｹﾌ ｾｲｷｹｵ蟆｡ ｽﾇﾇ狠ﾟﾀﾌｴﾙ...
#define		ERR_EVENT_FAILED_TO_LINK				-2706	// ﾀﾌｺ･ﾆｮ ｿｬｰ・ｽﾇﾆﾐ
#define		ERR_THREAD_WAIT_ABANDONED				-2707	// ｾｲｷｹｵ・ｵｿｱ・ｰｴﾃｼﾀｻ ｱ箒ﾙｸｮｴﾂ ｰﾍﾀｻ ﾆ簓ﾑｴﾙ. 
#define		ERR_THREAD_STOPPED						-2708	// ｾｲｷｹｵ蟆｡ ﾁﾟﾁﾈ ｻﾂﾀﾌｴﾙ.
#define		ERR_INVALID_EVENT_LIST					-2709	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾀﾌｺ･ﾆｮ ｸﾏ

#define		ERR_PARAMS_INVALID						-2725	// ﾀﾟｸﾈ ｸﾅｰｳｺｯｼ・
#define		ERR_PARAMS_RANGE						-2726	// ｸﾅｰｳｺｯｼﾇ ｹ・ｧｰ｡ ﾀﾟｸﾊ.
#define		ERR_PARAMS_LACK							-2727	// ｸﾅｰｳｺｯｼ・ｰｳｼ・ｺﾎﾁｷ
#define		ERR_PARAMS_UNKNOWN_TARGET				-2728	// ｸﾅｰｳｺｯｼ・ ｸ｣ｴﾂ ｴ・ﾓ.
#define		ERR_PARAMS_UNKNOWN_ITEMS				-2729	// ｸﾅｰｳｺｯｼ・ ｸ｣ｴﾂ ﾇﾗｸﾓ.
#define		ERR_PARAMS_UNSUPPORT					-2730	// ｸﾅｰｳｺｯｼ・ ﾁﾏﾁ・ｾﾊｴﾂ ｸﾅｰｳｺｯｼﾓ.
#define		ERR_PARAMS_NULL							-2731	// ｸﾅｰｳｺｯｼ｡ NULLﾀﾓ. 
#define		ERR_PARAMS_TOO_MANY						-2732	// ｸﾅｰｳｺｯｼ｡ ｳﾊｹｫ ｸｹﾀｽ.
#define		ERR_PARAMS_NONE							-2733	// ｸﾅｰｳｺｯｼ｡ ｾﾙ

#define		ERR_INVALID_VERSION						-2750	// ﾀﾟｸﾈ ｹ・
#define		ERR_UNKNOWN_VERSION		  ERR_INVALID_VERSION	// ｾﾋ ｼ・ｾﾂ ｹ・
#define		ERR_UNKNOWN_PLATFORM					-2751	// ｾﾋ ｼ・ｾﾂ ﾇﾃｷｧﾆ・
#define		ERR_INVALID_PLATFORM	  ERR_UNKNOWN_PLATFORM	// ﾀﾟｸﾈ ﾇﾃｷｧﾆ・
#define		ERR_INVALID_BOARDID		  ERR_UNKNOWN_PLATFORM
#define		ERR_UNKNOWN_BOARDID		  ERR_UNKNOWN_PLATFORM

#define		ERR_HEAP_PUSH							-2800	// ﾈ・ｽｺﾅﾃｿ｡ ｵ･ﾀﾌﾅﾍ ｳﾖｱ・PUSH) ｽﾇﾆﾐ
#define		ERR_HEAP_POP							-2801	// ﾈ・ｽｺﾅﾃｷﾎｺﾎﾅﾍ ｵ･ﾀﾌﾅﾍ ｰ｡ﾁｮｿﾀｱ・POP) ｽﾇﾆﾐ
#define		ERR_HEAP_FULL							-2802	// ﾈ・ｸﾞｸｮｰ｡ ｰ｡ｵ・ﾂ・
#define		ERR_HEAP_EMPTY							-2803	// ﾈ・ｹﾛｿ｡ ｵ･ﾀﾌﾅﾍｰ｡ ｾｽ.
#define		ERR_HEAP_ENQUEUE						-2804	// ﾈ・ﾅ･ｿ｡ ｵ･ﾀﾌﾅﾍ ｳﾖｱ・ENQUEUE) ｽﾇﾆﾐ 
#define		ERR_HEAP_DEQUEUE						-2805	// ﾈ・ﾅ･ｷﾎｺﾎﾅﾍ ｵ･ﾀﾌﾅﾍ ｰ｡ﾁｮｿﾀｱ・DEQUEUE) ｽﾇﾆﾐ 
#define		ERR_HEAP_PEEK							-2806	// ﾈ・ｸﾞｸｮｷﾎｺﾎﾅﾍ ﾇﾘｴ・ｻﾎ ｵ･ﾀﾌﾅﾍｸｦ ﾀﾐﾀｻ ｼ・ｾｽ.
#define		ERR_HEAP_INVALID_PARAM					-2807	// ﾈ・ ｸﾅｰｳｺｯｼ・ｿﾀｷ・
#define		ERR_HEAP_INVALID_INDEX					-2808	// ﾈ・ ﾀﾟｸﾈ ｻﾎ.
#define		ERR_HEAP_DELETE							-2809	// ﾈ・ｸﾞｸｮｷﾎｺﾎﾅﾍ ﾇﾘｴ・ｻﾎ ｵ･ﾀﾌﾅﾍｸｦ ﾁｦｰﾅ ｽﾇﾆﾐ
#define		ERR_HEAP_NOT_FOUND						-2810	// ﾈ・ｸﾞｸｮｿ｡ｼｭ ﾃ｣ｰ暲ﾚﾇﾏｴﾂ ｻﾎﾀﾇ ｳ・蟶ｦ ﾃ｣ﾁ・ｸﾔ.

#define		ERR_HEAP_ADD_LOCKED						-2822	// ﾈ・ ｵ･ﾀﾌﾅﾍ ﾃﾟｰ｡ ｶ・ｰﾉｸｲ.
#define		ERR_HEAP_REMOVE_LOCKED					-2823	// ﾈ・ ｵ･ﾀﾌﾅﾍ ﾁｦｰﾅ ｶ・ｰﾉｸｲ.

#define		ERR_SEARCH_INVALID_PARAM				-2825	// ﾀﾟｸﾈ ｰﾋｻ・ｸﾅｰｳ ｺｯｼ・
#define		ERR_BSEARCH_NOTFOUND					-2826	// ﾀﾌﾁ・ｰﾋｻ｡ｼｭ ﾇﾘｴ・ﾅｰｰｪ ｹﾟｰﾟ ｸﾔ. 
#define		ERR_NOT_FOUND							-2827	// ｵ･ﾀﾌﾅﾍｸｦ ﾃ｣ﾁ・ｸﾔ.
#define		ERR_INDEX_EXCEED						-2828	// ｻﾎﾀﾌ ｹ・ｧｸｦ ﾃﾊｰ戓ﾏｿｴﾀｽ.

#define		ERR_USB_ENDPOINT_INVALID_DIRECTION		-2892	// USB ﾀﾟｸﾈ ﾀﾔﾃ箙ﾂ ｹ貮・ｿ｣ｵ衄ﾎﾆｮ
#define		ERR_USB_URB_NULL						-2893	// USB URB ｸﾞｸｮ ﾆﾎﾅﾍ NULL
#define		ERR_USB_NOTFOUND_ENDPOINT				-2894	// USB ｿ｣ｵ衄ﾎﾅﾍｸｦ ﾃ｣ﾀｻ ｼ・ｾﾙ.
#define		ERR_USB_PIPES_INFORMATION_NULL			-2895	// USB PIPES INFORMATION ｸﾞｸｮ ﾆﾎﾅﾍ NULL
#define		ERR_USB_INTERFACE_INFORMATION_NULL		-2896	// USB INTERFACE INFORMATION ｸﾞｸｮ ﾆﾎﾅﾍ NULL
#define		ERR_USB_STRING_DESCRIPTOR				-2897	// USB String Descriptor ｿﾀｷ・
#define		ERR_USB_INVALID_DESCRIPTOR_TYPE			-2898	// USB ﾀﾟｸﾈ Descriptor ﾁｾｷ・
#define		ERR_USB_DESCRIPTOR						-2899	// USB Descriptor ｿﾀｷ・
#define		ERR_USB_DEVICE_DESCRIPTOR				-2900	// USB Device Descriptor ｿﾀｷ・
#define		ERR_USB_CONFIGURATION_DESCRIPTOR		-2901	// USB Configuration Descriptor ｿﾀｷ・
#define		ERR_USB_INTERFACE_DESCRIPTOR			-2902	// USB Interface Descriptor ｿﾀｷ・
#define		ERR_USB_ENDPOINT_DESCRIPTOR				-2903	// USB Endpoint Descriptor ｿﾀｷ・
#define		ERR_USB_SET_INTERFACE					-2904	// USB Interface ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_USB_CONFIGURATION_HANDLE			-2905	// USB Configuration ｿﾀｷ・
#define		ERR_USB_CONFIGURATING					-2906	// USB ﾀ蠧｡ ﾈｯｰ・ｼｳﾁ､ｿ｡ ﾀﾖﾀｽ...
#define		ERR_USB_UNCONFIGURATION					-2907	// USB Unconfiguration ｼｳﾁ､ ｽﾇﾆﾐ 
#define		ERR_USB_ABORT_PIPE						-2908	// USB ﾆﾄﾀﾌﾇﾁ ｻ鄙・ﾁﾟｴﾜ ｽﾇﾆﾐ

#define		ERR_FILE_HANLDE							-2920	// ﾆﾄﾀﾏ ﾇﾚｵ・ｿﾀｷ・
#define		ERR_FILE_GET_LENGTH						-2921	// ﾆﾄﾀﾏ ｱ貘ﾌ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FILE_OPEN							-2922	// ﾆﾄﾀﾏ ｿｭｱ・ｽﾇﾆﾐ
#define		ERR_FILE_NOT_OPEN				ERR_FILE_OPEN
#define		ERR_FILE_READ							-2923	// ﾆﾄﾀﾏ ﾀﾐｱ・ｽﾇﾆﾐ
#define		ERR_FILE_WRITE							-2924	// ﾆﾄﾀﾏ ｱ箙ﾏ ｽﾇﾆﾐ
#define		ERR_FILE_UNKNOWN_EXT					-2925	// ｾﾋ ｼ・ｾﾂ ﾆﾄﾀﾏ ﾈｮﾀ蠡ﾚ
#define		ERR_FILE_NULL							-2926	// ﾀﾟｸﾈ ﾆﾄﾀﾏ ﾆﾎﾅﾍ, NULL
#define		ERR_FILE_INVALID_SIZE					-2927	// ﾀﾟｸﾈ ﾆﾄﾀﾏ ﾅｩｱ・
#define		ERR_FILE_NOT_FOUND						-2928	// ﾆﾄﾀﾏ ﾃ｣ｱ・ｽﾇﾆﾐ
#define		ERR_FILE_NO_SELECTION					-2929	// ﾆﾄﾀﾏﾀﾌ ｼｱﾅﾃｵﾇﾁ・ｾﾊﾀｽ.

#define		ERR_MEM_NULL							-2930	// ｸﾞｸｮ ｹﾛ ｹ｡ NULLﾀﾌｴﾙ... ｸﾞｸｮ ﾇﾒｴ・ｽﾇﾆﾐ...
#define		ERR_MEM_ALLOCATED						-2931	// ｸﾞｸｮ ｹﾛｰ｡ ﾀﾌｹﾌ ﾇﾒｴ邨ﾇｾ・ﾀﾖﾀｽ.

#define		ERR_INVALID_PIPEDIRECTION	            -2984   // ﾀﾟｸﾈ USB ﾆﾄﾀﾌﾇﾁ ｹ貮・
#define		ERR_INVALID_PIPETYPE		            -2985   // ﾀﾟｸﾈ USB ﾆﾄﾀﾌﾇﾁ ﾁｾｷ・
#define		ERR_INVALID_PIPEINDEX		            -2986   // ﾀﾟｸﾈ USB ﾆﾄﾀﾌﾇﾁ ｻﾎ
#define		ERR_INVALID_PIPEHANDLE		            -2987   // ﾀﾟｸﾈ USB ﾆﾄﾀﾌﾇﾁ ﾇﾚｵ・
#define		ERR_INVALID_PIPEINFO		            -2988   // ﾀﾟｸﾈ USB ﾆﾄﾀﾌﾇﾁ ﾁ､ｺｸ
#define		ERR_INVALID_INTERFACEINFO	            -2989   // ﾀﾟｸﾈ USBﾀﾎﾅﾍﾆ菎ﾌｽｺ ﾁ､ｺｸ

#define		ERR_UNKNOWNPLATFORM			            -2990   // ｾﾋ ｼ・ｾﾂ ﾇﾃｷｧﾆ・
#define     ERR_INVALID_PARAMS 		   ERR_PARAMS_INVALID	// ｸﾅｰｳｺｯｼ・ｿﾀｷ・
#define     ERR_INVALID_PARAMS_RANGE   ERR_PARAMS_RANGE	    // ｸﾅｰｳｺｯｼ・ｹ・ｧ ｿﾀｷ・

#define		ERR_UNKNOWN_IR_DECODE_MODE			    -3116	// ｸ｣ｴﾂ IR ｵﾚｵ・ｹ貎ﾄ
#define		ERR_INVALID_IR_KEY					    -3117	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) IR ﾅｰｰｪ

#define		ERR_GET_KSDEVICE				        -4200	// KSDEVICE ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_KSDEVICE		ERR_GET_KSDEVICE
#define		ERR_GET_KSFILTER				        -4201	// KSFILTER ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_KSFILTER		ERR_GET_KSFILTER
#define		ERR_GET_KSPIN					        -4202	// KSPIN ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_KSPIN			ERR_GET_KSPIN
#define		ERR_GET_KSSTREAM_HEADER			        -4203	// KSSTREAM_HEADER ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_KSSTREAM_HEADER	ERR_GET_KSSTREAM_HEADER
#define		ERR_FAILED_TO_GET_KSSTREAM_POINTER	    -4204	// KSSTREAM_POINTER ｱｸﾇﾏｱ・ｽﾇﾆﾐ

#define		ERR_ANYSEESC_OPEN					        -4400	// ANYSEESC ｿｭｱ・ｽﾇﾆﾐ
#define		ERR_ANYSEESC_CLOSE					        -4401	// ANYSEESC ｴﾝｱ・ｽﾇﾆﾐ
#define		ERR_ANYSEESC_NOT_OPENED			        -4402	// ANYSEESC LIBｰ｡ ｿｭｷﾁﾀﾖﾁ・ｾﾊﾀｽ.
#define		ERR_ANYSEESC_ALREADY_OPENED		        -4403	// ANYSEESC LIBｰ｡ ﾀﾌｹﾌ ｿｭｷﾁﾀﾖﾀｽ.
#define		ERR_ANYSEESC_REOPEN				ERR_ANYSEESC_ALREADY_OPENED	
#define		ERR_ANYSEESC_FAILED_TO_LINK		        -4404	// ANYSEESC ｽｺｸｶﾆｮ ﾄｫｵ・ﾀﾎﾅﾍﾆ菎ﾌｽｺ ﾀ蠧｡ ﾇﾔｼ・ｿｬｰ・ｽﾇﾆﾐ
#define		ERR_ANYSEESC_NO_LINK				        -4405	// ANYSEESC ｽｺｸｶﾆｮ ﾄｫｵ・ﾀﾎﾅﾍﾆ菎ﾌｽｺ ﾀ蠧｡ ｿｬｰ・ﾇﾔｼ｡ ｾｽ.
#define		ERR_SMARTCARD_OPEN				        -4406	// ｽｺｸｶﾆｮ ﾄｫｵ・ﾀﾎﾅﾍﾆ菎ﾌｽｺ ﾀ蠧｡ ｿｭｱ・ｽﾇﾆﾐ
#define		ERR_SMARTCARD_CLOSE				        -4407	// ｽｺｸｶﾆｮ ﾄｫｵ・ﾀﾎﾅﾍﾆ菎ﾌｽｺ ﾀ蠧｡ ｴﾝｱ・ｽﾇﾆﾐ
#define		ERR_SMARTCARD_INITIALIZATION	        -4408	// ｽｺｸｶﾆｮ ﾄｫｵ・ﾃﾊｱ篳ｭ ｽﾇﾆﾐ
#define		ERR_ANYSEESC_FUNC					        -4409	// ANYSEESC ﾇﾔｼ・ﾈ｣ﾃ・ｽﾇﾆﾐ.
#define		ERR_ANYSEESC_SET_T0_TRANSMIT_MODE	        -4410	// T0 ﾇﾁｷﾎﾅ萋ﾝ ｼﾛｽﾅ ｹ貎ﾄ ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_ANYSEESC_SET_T0_RECEIVE_MODE	        -4411	// T0 ﾇﾁｷﾎﾅ萋ﾝ ｼﾅ ｹ貎ﾄ ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_ANYSEESC_SET_T1_TRANSMIT_MODE	        -4412	// T1 ﾇﾁｷﾎﾅ萋ﾝ ｼﾛｽﾅ ｹ貎ﾄ ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_ANYSEESC_SET_T1_RECEIVE_MODE	        -4413	// T1 ﾇﾁｷﾎﾅ萋ﾝ ｼﾅ ｹ貎ﾄ ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_ANYSEESC_SET_T14_TRANSMIT_MODE	        -4412	// T14 ﾇﾁｷﾎﾅ萋ﾝ ｼﾛｽﾅ ｹ貎ﾄ ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_ANYSEESC_SET_T14_RECEIVE_MODE	        -4413	// T14 ﾇﾁｷﾎﾅ萋ﾝ ｼﾅ ｹ貎ﾄ ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_ANYSEESC_CHECK_INTERRUPT		        -4414	// ﾀﾎﾅﾍｷｴﾆｮ ｰﾋｻ・ｽﾇﾆﾐ 
#define		ERR_ANYSEESC_T0_PARITY_ERROR		        -4415	// T0 ﾇﾁｷﾎﾅ萋ﾝ ﾆ莵ｯﾆｼ ｿﾀｷ・ｹﾟｻ
#define		ERR_ANYSEESC_NOT_INSERTED			        -4416	// ｽｺｸｶﾆｮ ﾄｫｵ蟆｡ ｻﾔｵﾇｾ・ﾀﾖﾁ・ｾﾊﾀｽ.
#define		ERR_ANYSEESC_T0_TRANSMIT_TIMEOUT	        -4417	// T0 ﾇﾁｷﾎﾅ萋ﾝ ｼﾛｽﾅ ｽﾃｰ｣ ﾃﾊｰ・
#define		ERR_ANYSEESC_T0_RECEIVE_TIMEOUT	        -4418	// T0 ﾇﾁｷﾎﾅ萋ﾝ ｼﾅ ｽﾃｰ｣ ﾃﾊｰ・
#define		ERR_ANYSEESC_T1_TRANSMIT_TIMEOUT	        -4419	// T1 ﾇﾁｷﾎﾅ萋ﾝ ｼﾛｽﾅ ｽﾃｰ｣ ﾃﾊｰ・
#define		ERR_ANYSEESC_T1_RECEIVE_TIMEOUT	        -4420	// T1 ﾇﾁｷﾎﾅ萋ﾝ ｼﾅ ｽﾃｰ｣ ﾃﾊｰ・
#define		ERR_ANYSEESC_T14_TRANSMIT_TIMEOUT	        -4421	// T14 ﾇﾁｷﾎﾅ萋ﾝ ｼﾛｽﾅ ｽﾃｰ｣ ﾃﾊｰ・
#define		ERR_ANYSEESC_T14_RECEIVE_TIMEOUT	        -4422	// T14 ﾇﾁｷﾎﾅ萋ﾝ ｼﾅ ｽﾃｰ｣ ﾃﾊｰ・
#define		ERR_ANYSEESC_INVALID_CLASS			        -4423	// ｹｫﾈｿﾇﾑ ﾅｬｷ｡ｽｺ
#define		ERR_ANYSEESC_INVALID_CODE			        -4424	// ｹｫﾈｿﾇﾑ ﾄﾚｵ・
#define		ERR_ANYSEESC_INCORRECT_REFERENCE	        -4425	// ﾀﾟｸﾈ ﾂ・ｶ
#define		ERR_ANYSEESC_INCORRECT_LENGTH		        -4426	// ﾀﾟｸﾈ ｱ貘ﾌ
#define		ERR_ANYSEESC_UNKNOWN_SW1			        -4427	// ｸ｣ｴﾂ ｻﾂ ｿ・ｹﾙﾀﾌﾆｮ1 
#define		ERR_ANYSEESC_UNKNOWN_SW2			        -4428	// ｸ｣ｴﾂ ｻﾂ ｿ・ｹﾙﾀﾌﾆｮ2
#define		ERR_ANYSEESC_INVALID_STATUS_BYTE	        -4429	// ｹｫﾈｿﾇﾑ ｻﾂ ｹﾙﾀﾌﾆｮ
#define		ERR_ANYSEESC_INCORRECT_COMMAND_HEADER	    -4430	// ﾀﾟｸﾈ ｸ昞ﾉｾ・ﾇ・・
#define		ERR_ANYSEESC_SET_INTERRUPT				    -4431	// ﾀﾎﾅﾍｷｴﾆｮ ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_ANYSEESC_ATR_TS_NOT_RECEIVED		    -4432	// FWT ﾅｬｷｰｳｻｿ｡ ATR TS ｹﾙﾀﾌﾆｮｸｦ ｹﾞﾁ・ｸﾔ.
#define		ERR_ANYSEESC_ATR_TS_TIMEOUT			    -4433	// ATR TS ｹﾞｱ・ｽﾃｰ｣ ﾃﾊｰ・
#define		ERR_ANYSEESC_ATR_INCORRECT_TS			    -4434	// ﾀﾟｸﾈ ATR TS ｹﾙﾀﾌﾆｮ
#define		ERR_ANYSEESC_ATR_BWT_TIMEOUT			    -4444	// ATR BWT ｽﾃｰ｣ ﾃﾊｰ・
#define		ERR_ANYSEESC_ATR_DATASTREAM_TIMEOUT	    -4445	// ATR ｵ･ﾀﾌﾅﾍ ｽｺﾆｮｸｲ ｽﾃｰ｣ ﾃﾊｰ・
#define		ERR_ANYSEESC_ATR_TCK_CHECKSUM			    -4446	// TCK(Check Character) ﾃｼﾅｩｼｶ ｿﾀｷ・
#define		ERR_ANYSEESC_ATR_TIMEOUT				    -4447	// ATR ｽﾃﾄｺ ｽﾃｰ｣ ﾃﾊｰ・
#define		ERR_ANYSEESC_ATR_INCORRECT_DATASTREAM	    -4448	// ﾀﾟｸﾈ ATR ｵ･ﾀﾌﾅﾍ ｽｺﾆｮｸｲ
#define		ERR_ANYSEESC_ATR_INVALID_TA1			    -4449	// ｹｫﾈｿﾇﾑ ATR TA1 ｹﾙﾀﾌﾆｮ 
#define		ERR_ANYSEESC_ATR_INVALID_TB1			    -4450	// ｹｫﾈｿﾇﾑ ATR TB1 ｹﾙﾀﾌﾆｮ 
#define		ERR_ANYSEESC_ATR_INVALID_TC1			    -4451	// ｹｫﾈｿﾇﾑ ATR TC1 ｹﾙﾀﾌﾆｮ 
#define		ERR_ANYSEESC_ATR_INVALID_TD1			    -4452	// ｹｫﾈｿﾇﾑ ATR TD1 ｹﾙﾀﾌﾆｮ 
#define		ERR_ANYSEESC_ATR_INVALID_TA2			    -4453	// ｹｫﾈｿﾇﾑ ATR TA2 ｹﾙﾀﾌﾆｮ 
#define		ERR_ANYSEESC_ATR_INVALID_TB2			    -4454	// ｹｫﾈｿﾇﾑ ATR TB2 ｹﾙﾀﾌﾆｮ 
#define		ERR_ANYSEESC_ATR_INVALID_TC2			    -4455	// ｹｫﾈｿﾇﾑ ATR TC2 ｹﾙﾀﾌﾆｮ 
#define		ERR_ANYSEESC_ATR_INVALID_TD2			    -4456	// ｹｫﾈｿﾇﾑ ATR TD2 ｹﾙﾀﾌﾆｮ 
#define		ERR_ANYSEESC_ATR_INVALID_TA3			    -4457	// ｹｫﾈｿﾇﾑ ATR TA3 ｹﾙﾀﾌﾆｮ 
#define		ERR_ANYSEESC_ATR_INVALID_TB3			    -4458	// ｹｫﾈｿﾇﾑ ATR TB3 ｹﾙﾀﾌﾆｮ 
#define		ERR_ANYSEESC_ATR_INVALID_TC3			    -4459	// ｹｫﾈｿﾇﾑ ATR TC3 ｹﾙﾀﾌﾆｮ 
#define		ERR_ANYSEESC_ATR_INVALID_TD3			    -4460	// ｹｫﾈｿﾇﾑ ATR TD3 ｹﾙﾀﾌﾆｮ 
#define		ERR_ANYSEESC_ATR_INVALID_TA4			    -4461	// ｹｫﾈｿﾇﾑ ATR TA4 ｹﾙﾀﾌﾆｮ 
#define		ERR_ANYSEESC_ATR_INVALID_TB4			    -4462	// ｹｫﾈｿﾇﾑ ATR TB4 ｹﾙﾀﾌﾆｮ 
#define		ERR_ANYSEESC_ATR_INVALID_TC4			    -4463	// ｹｫﾈｿﾇﾑ ATR TC4 ｹﾙﾀﾌﾆｮ 
#define		ERR_ANYSEESC_ATR_INVALID_TD4			    -4464	// ｹｫﾈｿﾇﾑ ATR TD4 ｹﾙﾀﾌﾆｮ 
#define		ERR_ANYSEESC_ATR_INCORRECT_FD_FACTOR	    -4465	// ﾀﾟｸﾈ ATR F, D ｰ霈・
#define		ERR_ANYSEESC_ATR_UNKNOWN_PROTOCOL		    -4466	// ATR, ｸ｣ｴﾂ ﾇﾁｷﾎﾅ萋ﾝ
#define		ERR_ANYSEESC_ATR_UNSUPPORT_PROTOCOL	    -4467	// ATR, ﾁﾏﾁ・ｾﾊｴﾂ ﾇﾁｷﾎﾅ萋ﾝ
#define		ERR_ANYSEESC_ATR_SET_PARAMS			    -4468	// ATR ｸﾅｰｳｺｯｼ・ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_ANYSEESC_SET_ERROR_RETRYCOUNT		    -4469	// ｿﾀｷ・ﾀ鄲・ﾛ ｽﾃｵｵ ﾈｸｼ・ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_ANYSEESC_T0_INCORRECT_LENGTH		    -4470	// T0 ﾇﾁｷﾎﾅ萋ﾝ, ﾀﾟｸﾈ ｵ･ﾀﾌﾅﾍ ｱ貘ﾌ
#define		ERR_ANYSEESC_NOT_READY_TO_READWRITE_SLOT0	-4471	// ｽｽｷﾔ0, ｽｺｸｶﾆｮ ﾄｫｵ・ｵ･ﾀﾌﾅﾍ ﾀﾐｱ・ｾｲｱ簓ﾒ ﾁﾘｺ｡ ｾﾈｵﾇｾ・ﾀﾖﾀｽ.
#define		ERR_ANYSEESC_NOT_READY_TO_READWRITE_SLOT1	-4472	// ｽｽｷﾔ1, ｽｺｸｶﾆｮ ﾄｫｵ・ｵ･ﾀﾌﾅﾍ ﾀﾐｱ・ｾｲｱ簓ﾒ ﾁﾘｺ｡ ｾﾈｵﾇｾ・ﾀﾖﾀｽ.
#define		ERR_ANYSEESC_PPS_FAILED					-4473	// PPS ｽﾇﾆﾐ
#define		ERR_ANYSEESC_ATR_INVALID_TKCOUNT			-4474	// ﾀﾟｸﾈ TK ｱ貘ﾌ
#define		ERR_ANYSEESC_FWUSER_STATUS					-4477	// ﾆﾟｿｾ・ｻ鄙・ﾚ ｻﾂ ｿﾀｷ・
#define		ERR_ANYSEESC_INTERRUPT_FAILURE				-4478	// ANYSEESC ﾀﾎﾅﾍｷｴﾆｮ ｽﾇﾆﾐ
#define		ERR_ANYSEESC_FAILED_TO_READ_REG			-4479	// ｽｺｸｶﾆｮﾄｫｵ・ﾀﾎﾅﾍﾆ菎ﾌｽｺ IC ｷｹﾁｺﾅﾍ ﾀﾐｱ・ｽﾇﾆﾐ
#define		ERR_ANYSEESC_FAILED_TO_WRITE_REG			-4480	// ｽｺｸｶﾆｮﾄｫｵ・ﾀﾎﾅﾍﾆ菎ﾌｽｺ IC ｷｹﾁｺﾅﾍ ｾｲｱ・ｽﾇﾆﾐ
#define		ERR_ANYSEESC_FAILED_TO_SET_CMDVCC			-4481	// CMDVCC(ｽｺｸｶﾆｮﾄｫｵ・ﾀ・・ｰﾞ) ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_ANYSEESC_FAILED_TO_INITIALIZE		ERR_SMARTCARD_INITIALIZATION
#define		ERR_ANYSEESC_T14_INCORRECT_SEND_LENGTH		-4482	// T14 ﾇﾁｷﾎﾅ萋ﾝ, ﾀﾟｸﾈ ｺｸｳｻｱ・ｵ･ﾀﾌﾅﾍ ｱ貘ﾌ
#define		ERR_ANYSEESC_T14_INCORRECT_RESPONSE		-4483	// T14 ﾇﾁｷﾎﾅ萋ﾝ, ﾀﾟｸﾈ ﾀﾀｴ・
#define		ERR_ANYSEESC_T14_INCORRECT_RESPONSE_LENGTH	-4484	// T14 ﾇﾁｷﾎﾅ萋ﾝ, ﾀﾟｸﾈ ﾀﾀｴ・ｵ･ﾀﾌﾅﾍ ｱ貘ﾌ
#define		ERR_ANYSEESC_T14_INCORRECT_RESPONSE_XOR_CHECKSUM	-4485	// T14 ﾇﾁｷﾎﾅ萋ﾝ, ﾀﾟｸﾈ ﾀﾀｴ・ｵ･ﾀﾌﾅﾍ, ｵ･ﾀﾌﾅﾍ ｹﾞｱ・ｿﾀｷ・ｹﾟｻ.

#define		ERR_DX_FAILED_TO_CREATE_FILTER_GRAPH		-7000	// DirectShow, ﾇﾊﾅﾍ ｱﾗｷ｡ﾇﾁ ｻｼｺ ｽﾇﾆﾐ
#define		ERR_DX_INVALID_FILTER_GRAPH					-7001	// DirectShow, ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾇﾊﾅﾍ ｱﾗｷ｡ﾇﾁ
#define		ERR_DX_FAILED_TO_SET_KSPROPERTYSET			-7002	// DirectShow, KsPropertySet ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_KSPROPERTYSET			-7003	// DirectShow, KsPropertySet ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_COCREATE_ICreateDevEnum	-7004	// DirectShow, ICreateDevEnum ﾀﾎｽｺﾅﾏｽｺ ｻｼｺ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_CREATE_CLASS_ENUMERATOR	-7005	// DirectShow, Class Enumerator ｻｼｺ ｽﾇﾆﾐ
#define		ERR_DX_NOT_FOUND_CLASS						-7006	// DirectShow, Class Enumeratorｷﾎｺﾎﾅﾍ ﾇﾘｴ・ﾅｬｷ｡ｽｺｰ｡ ｹﾟｰﾟｵﾇﾁ・ｾﾊﾀｽ. 
#define		ERR_DX_FAILED_TO_GET_MONIKER_STORAGE		-7007	// DirectShow, Monikerﾀﾇ ﾀ惕・ﾆﾎﾅﾍ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_ADD_FILTER					-7008	// DirectShow, ﾇﾊﾅﾍ ｱﾗｷ｡ﾇﾁｿ｡ ﾇﾊﾅﾍ ﾃﾟｰ｡ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_ENUMERATE_UPSTREAM_FILTER_PINS		-7009	// DirectShow, ｻｧ ｽｺﾆｮｸｲ ﾇﾊﾅﾍ ﾇﾉ ｳｪｿｭ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_ENUMERATE_DOWNSTREAM_FILTER_PINS	-7010	// DirectShow, ﾇﾏﾀｧ ｽｺﾆｮｸｲ ﾇﾊﾅﾍ ﾇﾉ ｳｪｿｭ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_ENUMERATE_FILTER_PINS		-7011	// DirectShow, ﾇﾊﾅﾍ ﾇﾉ ｳｪｿｭ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_QUERY_PIN_INFORMATION		-7012	// DirectShow, ﾇﾊﾅﾍ ﾇﾉ ﾁ､ｺｸ ﾁ惕ﾇ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_QUERY_PIN_DIRECTION		-7013	// DirectShow, ﾇﾊﾅﾍ ﾇﾉ ｹ貮・ﾁ惕ﾇ ｽﾇﾆﾐ
#define		ERR_DX_INVALID_PIN_DIRECTION				-7014	// DirectShow, ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾇﾉ ｹ貮・
#define		ERR_DX_FAILED_TO_CONNECT_PINS				-7015	// DirectShow, ﾇﾉ ｿｬｰ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_CREATE_TUNER_SPACE			-7016	// DirectShow, ﾆｩｴﾗ ｰ｣(Tuning Space) ｻｼｺ ｽﾇﾆﾐ
#define		ERR_DX_INVALID_TUNER_SPACE					-7017	// DirectShow, ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾆｩｳﾊ ｽｺﾆ菎ﾌｽｺ
#define		ERR_DX_FAILED_TO_GET_BDA_NETWORK_TYPE		-7018	// DirectShow, BDA ｳﾗﾆｮｿｩ ﾁｾｷ・ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_CLSID					-7019	// DirectShow, CLSID ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_CREATE_BDA_NETWORK_PROVIDER	-7020	// DirectShow, BDA ｳﾗﾆｮｿｩ ﾇﾁｷﾎｹﾙﾀﾌﾀ・ｻｼｺ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_TUNING_SPACE_CONTAINER		-7021	// DirectShow, ﾆｩｴﾗ ｰ｣(Tuning Space) ﾄﾜﾅﾗﾀﾎｳﾊ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_TUNING_SPACE_ENUMERATOR	-7022	// DirectShow, ﾆｩｴﾗ ｰ｣(Tuning Space) ｳｪｿｭﾀﾚ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_INVALID_TUNING_SPACE_CONTAINER			-7023	// DirectShow, ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾆｩｴﾗ ｰ｣(Tuning Space) ﾄﾜﾅﾗﾀﾎｳﾊ
#define		ERR_DX_FAILED_TO_ENUMERATE_TUNING_SPACE			-7024	// DirectShow, ﾆｩｴﾗ ｰ｣(Tuning Space) ｳｪｿｭ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_READ_TUNING_SPACE				-7025	// DirectShow, ﾆｩｴﾗ ｰ｣(Tuning Space) ﾀﾐｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_CLONE_TUNING_SPACE				-7026	// DirectShow, ﾆｩｴﾗ ｰ｣(Tuning Space) ｺｹﾁｦ ｽﾇﾆﾐ
#define		ERR_DX_INVALID_TUNING_SPACE						-7027	// DirectShow, ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾆｩｴﾗ ｰ｣(Tuning Space)
#define		ERR_DX_FAILED_TO_CREATE_ATSC_TUNING_SPACE		-7028	// DirectShow, ATSC ﾆｩｴﾗ ｰ｣(Tuning Space) ｻｼｺ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_CREATE_DVBT_TUNING_SPACE		-7029	// DirectShow, DVB-T ﾆｩｴﾗ ｰ｣(Tuning Space) ｻｼｺ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_CREATE_DVBC_TUNING_SPACE		-7030	// DirectShow, DVB-C ﾆｩｴﾗ ｰ｣(Tuning Space) ｻｼｺ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_CREATE_DVBS_TUNING_SPACE		-7031	// DirectShow, DVB-S ﾆｩｴﾗ ｰ｣(Tuning Space) ｻｼｺ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_FREQUENCY_MAPPING			-7032	// DirectShow, ﾁﾖﾆﾄｼ・ｸﾊﾇﾎ ｳﾖｱ・ｼｳﾁ､) ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_UNIQUE_NAME				-7033	// DirectShow, Unique ﾀﾌｸｧ ｳﾖｱ・ｼｳﾁ､) ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_DVB_SYSTEM_TYPE			-7034	// DirectShow, DVB ｹ貍ﾛ ｽﾃｽｺﾅﾛ ﾁｾｷ・ｳﾖｱ・ｼｳﾁ､) ｽﾇﾆﾐ 
#define		ERR_DX_FAILED_TO_PUT_BDA_NETWORK_TYPE			-7035	// DirectShow, BDA ｳﾗﾆｮｿｩ ﾁｾｷ・ｳﾖｱ・ｼｳﾁ､) ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_FRIENDLY_NAME				-7036	// DirectShow, FriendlyName ｳﾖｱ・ｼｳﾁ､) ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_CREATE_ATSC_LOCATOR			-7037	// DirectShow, ATSC Locator ｻｼｺ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_CREATE_DVBT_LOCATOR			-7038	// DirectShow, DVB-T Locator ｻｼｺ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_CREATE_DVBC_LOCATOR			-7039	// DirectShow, DVB-C Locator ｻｼｺ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_CREATE_DVBS_LOCATOR			-7040	// DirectShow, DVB-S Locator ｻｼｺ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_CREATE_TUNING_SPACE_CONTAINER	-7041	// DirectShow, ﾆｩｴﾗ ｰ｣(Tuning Space) ﾄﾜﾅﾗﾀﾎｳﾊ ｻｼｺ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_TUNING_SPACES_COUNT		-7042	// DirectShow, ﾆｩｴﾗ ｰ｣(Tuning Space) ｰｳｼ・ｱｸﾇﾏｱ・
#define		ERR_DX_FAILED_TO_ADD_TUNING_SPACE				-7043	// DirectShow, ﾆｩｴﾗ ｰ｣(Tuning Space) ﾃﾟｰ｡ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_ADD_BDA_NETWORK_PROVIDER		-7044	// DirectShow, BDA ｳﾗﾆｮｿｩ ﾇﾁｷﾎｹﾙﾀﾌﾀ・ﾇﾊﾅﾍ ﾃﾟｰ｡ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_ADD_BDA_TUNER					-7045	// DirectShow, BDA ﾆｩｳﾊ ﾇﾊﾅﾍ ﾃﾟｰ｡ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_ADD_BDA_CAPTURE				-7046	// DirectShow, BDA ﾄｸﾃﾄ ﾇﾊﾅﾍ ﾃﾟｰ｡ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_ADD_MPEG2_DEMUX				-7047	// DirectShow, MPEG-2 Demultiplexer ﾀ滅ﾍ ﾃﾟｰ｡ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_ADD_BDA_MPEG2_TIF				-7048	// DirectShow, BDA MPEG-2 Transport Information ﾀ滅ﾍ ﾃﾟｰ｡ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_ADD_MPEG2_SECTION_TABLE		-7049	// DirectShow, MPEG-2 Section and Tables ﾀ滅ﾍ ﾃﾟｰ｡ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_QUERY_ISCANNINGTUNER			-7050	// DirectShow, IScanningTuner ﾀﾎﾅﾍﾆ菎ﾌｽｺ ﾁ惕ﾇ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_CREATE_TUNING_SPACE			-7051	// DirectShow, ﾆｩｴﾗ ｰ｣(Tuning Space) ｻｼｺ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_CREATE_TUNE_REQUEST			-7052	// DirectShow, ﾆｪ ｸｮﾄｺﾆｮ ｻｼｺ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_QUERY_ATSC_TUNE_REQUEST		-7053	// DirectShow, ATSC ﾆｪ ｸｮﾄｺﾆｮ ﾁ惕ﾇ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_QUERY_DVB_TUNE_REQUEST			-7054	// DirectShow, DVB ﾆｪ ｸｮﾄｺﾆｮ ﾁ惕ﾇ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_CARRIER_FREQUENCY			-7055	// DirectShow, ｹ貍ﾛ ﾁﾖﾆﾄｼ・ｳﾖｱ・ｼｳﾁ､) ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_BANDWIDTH					-7056	// DirectShow, ｹ貍ﾛ ﾁﾖﾆﾄｼ・ｴ・ｪ ｳﾖｱ・ｼｳﾁ､) ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_SYMBOLRATE					-7057	// DirectShow, ｽﾉｹ・ｷｹﾀﾌﾆｮ ｳﾖｱ・ｼｳﾁ､) ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_LNB_POLARISATION			-7058	// DirectShow, LNB ﾆ枻ﾄ ｱﾘｼｺ ｳﾖｱ・ｼｳﾁ､) ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_LOCATOR					-7059	// DirectShow, Locator ｳﾖｱ・ｼｳﾁ､) ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_DVBT_LOCATOR				-7060	// DirectShow, DVB-T Locator ｳﾖｱ・ｼｳﾁ､) ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_DVBC_LOCATOR				-7061	// DirectShow, DVB-C Locator ｳﾖｱ・ｼｳﾁ､) ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_DVBS_LOCATOR				-7062	// DirectShow, DVB-S Locator ｳﾖｱ・ｼｳﾁ､) ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_ATSC_LOCATOR				-7063	// DirectShow, ATSC Locator ｳﾖｱ・ｼｳﾁ､) ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_DVB_TUNE_REQUEST			-7064	// DirectShow, DVB ﾆｪ ｸｮﾄｺﾆｮ ｳﾖｱ・ｼｳﾁ､) ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_ENUMERATE_MEDIA_TYPES			-7065	// DirectShow, ｹﾌｵ・ﾁｾｷ・ｳｪｿｭ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_RESET_MEDIA_TYPES				-7066	// DirectShow, ｹﾌｵ・ﾁｾｷ・ｳｪｿｭ ｸｮｼﾂ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_QUERY_AUDIO_PID_MAP			-7067	// DirectShow, MPEG-2 Demux ｿﾀｵﾀ PID MAP ﾁ惕ﾇ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_QUERY_VIDEO_PID_MAP			-7068	// DirectShow, MPEG-2 Demux ｺﾀ PID MAP ﾁ惕ﾇ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_MAP_AUDIO_PID					-7069	// DirectShow, MPEG-2 Demux ｿﾀｵﾀ PID ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_MAP_VIDEO_PID					-7070	// DirectShow, MPEG-2 Demux ｺﾀ PID ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PAUSE_MEDIA_CONTROL			-7071	// DirectShow, IMediaControl ﾀﾏｽﾃ ﾁﾟﾁ・Pause) ｽﾇﾆﾐ 
#define		ERR_DX_FAILED_TO_STOP_MEDIA_CONTROL				-7072	// DirectShow, IMediaControl ﾁﾟﾁ・Stop) ｽﾇﾆﾐ 
#define		ERR_DX_FAILED_TO_RUN_MEDIA_CONTROL				-7073	// DirectShow, IMediaControl ｽﾇﾇ・Run) ｽﾇﾆﾐ 
#define		ERR_DX_INVALID_MEDIA_CONTROL					-7074	// DirectShow, ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) IMediaControl
#define		ERR_DX_FAILED_TO_ADD_MPEG_AUDIO_DECODER			-7075	// DirectShow, MPEG Audio Decoder ﾇﾊﾅﾍ ﾃﾟｰ｡ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_ADD_MPEG_VIDEO_DECODER			-7076	// DirectShow, MPEG Video Decoder ﾇﾊﾅﾍ ﾃﾟｰ｡ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_QUERY_INTERFACE				-7077	// DirectShow, QueryInterface() ｿﾀｷ・
#define		ERR_DX_FAILED_TO_FIND_TUNING_SPACE_ID			-7078	// DirectShow, ﾆｪｴﾗ ｰ｣(Tuning Space) ID ﾃ｣ｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_THE_NUMBER_OF_TUNING_SPACE	-7079	// DirectShow, ﾆｪｴﾗ ｰ｣(Tuning Space) ｰｳｼ・ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_REMOVE_TUNING_SPACE			-7080	// DirectShow, ﾆｪｴﾗ ｰ｣(Tuning Space) ﾁｦｰﾅ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_IKSPROPERTYSET				-7081	// DirectShow, ﾄｿｳﾎ ﾇﾊﾅﾍ ｼﾓｼｺ ﾀﾎﾅﾍﾆ菎ﾌｽｺ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_QUERY_PID_MAP					-7082	// DirectShow, PID MAP ﾁ惕ﾇ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_MAP_PID						-7083	// DirectShow, PID ｸﾊﾇﾎ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_UNMAP_PID						-7084	// DirectShow, PID ｸﾊﾇﾎｵﾈ PID ﾁｦｰﾅ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_IBDA_TOPOLOGY				-7085	// DirectShow, BDA ﾅ菷弡ﾎﾁ・ﾀﾎﾅﾍﾆ菎ﾌｽｺ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_BDA_TOPOLOGY_NODE_TYPES	-7086	// DirectShow, BDA ﾅ菷弡ﾎﾁ・ｳ・・ﾁｾｷ・ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_BDA_TOPOLOGY_CONTROL_NODE	-7087	// DirectShow, BDA ﾅ菷弡ﾎﾁ・ﾁｦｾ・ｳ・・ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_IBDA_RFTUNER_PROPERTY		-7088	// DirectShow, BDA RF ﾆｩｳﾊ (ｼﾓｼｺ) ﾀﾎﾅﾍﾆ菎ﾌｽｺ(IID_IBDA_FrequencyFilter) ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_BDA_RFTUNER_PROPERTY		-7089	// DirectShow, BDA RF ﾆｩｳﾊ ｼﾓｼｺ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_INVALID_BDA_TUNER_FILTER_PROPERTY_ID		-7090	// DirectShow, ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) BDA ﾆｩｳﾊ ﾇﾊﾅﾍ ｼﾓｼｺ ID ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_ILOCATOR					-7091	// DirectShow, ILocator ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_ATSC_LOCATOR				-7092	// DirectShow, ATSC Locator ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_DVBT_LOCATOR				-7093	// DirectShow, DVB-T Locator ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_DVBC_LOCATOR				-7094	// DirectShow, DVB-C Locator ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_DVBS_LOCATOR				-7095	// DirectShow, DVB-S Locator ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_ATSC_TUNING_SPACE			-7096	// DirectShow, ATSC ﾆｩｴﾗ ｰ｣(Tuning Space) ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_DVBT_TUNING_SPACE			-7097	// DirectShow, DVB-T ﾆｩｴﾗ ｰ｣(Tuning Space) ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_DVBC_TUNING_SPACE			-7098	// DirectShow, DVB-C ﾆｩｴﾗ ｰ｣(Tuning Space) ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_DVBS_TUNING_SPACE			-7099	// DirectShow, DVB-S ﾆｩｴﾗ ｰ｣(Tuning Space) ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_IBDA_FREQUENCY_FILTER		-7080	// DirectShow, BDA RF ﾆｩｳﾊ ﾁﾖﾆﾄｼ・ﾇﾊﾅﾍ ﾀﾎﾅﾍﾆ菎ﾌｽｺ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_IBDA_LNB_INFO				-7081	// DirectShow, BDA LNB ﾁ､ｺｸ ﾀﾎﾅﾍﾆ菎ﾌｽｺ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_IBDA_DIGITAL_DEMODULATOR	-7082	// DirectShow, BDA ｵﾐ ｺｯﾁｶｱ・ﾀﾎﾅﾍﾆ菎ﾌｽｺ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_BDA_TUNER_FILTER_FREQUENCY_PROPERTY	-7083	// DirectShow, BDA ﾆｩｳﾊ ﾇﾊﾅﾍ ﾁﾖﾆﾄｼ・ｼﾓｼｺ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_BDA_TUNER_FILTER_FREQUENCY_PROPERTY	-7084	// DirectShow, BDA ﾆｩｳﾊ ﾇﾊﾅﾍ ﾁﾖﾆﾄｼ・ｼﾓｼｺ ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_BDA_LNB_INFO_PROPERTY					-7085	// DirectShow, BDA ﾆｩｳﾊ ﾇﾊﾅﾍ LNB ﾁ､ｺｸ ｼﾓｼｺ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_BDA_LNB_INFO_PROPERTY					-7086	// DirectShow, BDA ﾆｩｳﾊ ﾇﾊﾅﾍ LNB ﾁ､ｺｸ ｼﾓｼｺ ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_BDA_DIGITIAL_DEMODULATOR_PROPERTY		-7087	// DirectShow, BDA ﾆｩｳﾊ ﾇﾊﾅﾍ ｵﾐ ｺｯﾁｶｱ・ｼﾓｼｺ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_BDA_DIGITIAL_DEMODULATOR_PROPERTY		-7088	// DirectShow, BDA ﾆｩｳﾊ ﾇﾊﾅﾍ ｵﾐ ｺｯﾁｶｱ・ｼﾓｼｺ ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_DX_INVALID_ITUNER							-7089	// DirectShow, ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾆｩｳﾊ ﾀﾎﾅﾍﾆ菎ﾌｽｺ
#define		ERR_DX_INVALID_BDA_TOPOLOGY_NODE_TYPE_INDEX		-7090	// DirectShow, ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) BDA ﾅ菷弡ﾎﾁ・ｳ・・ﾁｾｷ・ｻﾎ
#define		ERR_DX_FAILED_TO_PUT_TUNE_REQUEST				-7091	// DirectShow, ﾆｪ ｿ菘ｻ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_ATSC_TUNE_REQUEST			-7092	// DirectShow, ATSC ﾆｪ ｿ菘ｻ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_DVBT_TUNE_REQUEST			-7093	// DirectShow, DVB-T ﾆｪ ｿ菘ｻ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_DVBC_TUNE_REQUEST			-7094	// DirectShow, DVB-C ﾆｪ ｿ菘ｻ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_DVBS_TUNE_REQUEST			-7095	// DirectShow, DVB-S ﾆｪ ｿ菘ｻ ｽﾇﾆﾐ
#define		ERR_DX_INVALID_TUNE_REQUEST						-7096	// DirectShow, ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾆｪ ｿ菘ｻ
#define		ERR_DX_INVALID_ATSC_TUNE_REQUEST				-7097	// DirectShow, ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ATSC ﾆｪ ｿ菘ｻ
#define		ERR_DX_INVALID_DVB_TUNE_REQUEST					-7098	// DirectShow, ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) DVB ﾆｪ ｿ菘ｻ
#define		ERR_DX_INVALID_DVBT_TUNE_REQUEST				-7099	// DirectShow, ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) DVB-T ﾆｪ ｿ菘ｻ
#define		ERR_DX_INVALID_DVBC_TUNE_REQUEST				-7100	// DirectShow, ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) DVB-C ﾆｪ ｿ菘ｻ
#define		ERR_DX_INVALID_DVBS_TUNE_REQUEST				-7101	// DirectShow, ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) DVB-S ﾆｪ ｿ菘ｻ
#define		ERR_DX_FAILED_TO_GET_IBDA_SIGNAL_STATISTICS		-7102	// DirectShow, BDA ｽﾅﾈ｣ ﾅ・・ﾀﾎﾅﾍﾆ菎ﾌｽｺ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_SIGNAL_SAMPLE_TIME			-7103	// DirectShow, ｽﾅﾈ｣ ｻﾃ ｽﾃｰ｣ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_SIGNAL_LOCKED				-7104	// DirectShow, ｽﾅﾈ｣ ﾀ篳・ｿｩｺﾎ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_SIGNAL_PRESENT				-7105	// DirectShow, ｽﾅﾈ｣ ﾁｸﾀ・ｿｩｺﾎ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_SIGNAL_QUALITY				-7106	// DirectShow, ｽﾅﾈ｣ ｰｨｵｵ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_GET_SIGNAL_STRENGTH			-7107	// DirectShow, ｽﾅﾈ｣ ｼｼｱ・ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DX_INVALID_PROPERTY_ID						-7108	// DirectShow, ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｼﾓｼｺ Id
#define		ERR_DX_UNSUPPORT_PROPERTY_ID					-7109	// DirectShow, ﾁﾏﾁ・ｾﾊｴﾂ ｼﾓｼｺ Id
#define		ERR_DX_INSUFFICIENT_BUFFER						-7110	// DirectShow, ｹﾛｰ｡ ｺﾎﾁｷﾇﾏｴﾙ.
#define		ERR_DX_OPENED_PIN								-7111	// DirectShow, ﾇﾉﾀﾌ ﾀﾌｹﾌ ｿｭｸｰ ｻﾂﾀﾌｴﾙ.
#define		ERR_DX_FAILED_TO_ENUMERATE_FILTERS				-7112	// DirectShow, ﾇﾊﾅﾍ ｳｪｿｭ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_BUILD_FILTER_GRAPH				-7113	// DirectShow, ﾇﾊﾅﾍ ｱﾗｷ｡ﾇﾁ ｸｸｵ魍・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_ITEM						-7114	// DirectShow, ﾇﾗｸ・ｳﾖｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_UPDATE_TUNING_SPACE			-7115	// DirectShow, ﾆｩｴﾗ ｰ｣(Tuning Space) ｰｻｽﾅ ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_TUNING_SPACE				-7116	// DirectShow, ﾆｩｴﾗ ｰ｣(Tuning Space) ｳﾖｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_ATSC_TUNING_SPACE			-7117	// DirectShow, ATSC ﾆｩｴﾗ ｰ｣(Tuning Space) ｳﾖｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_DVBT_TUNING_SPACE			-7118	// DirectShow, DVB-T ﾆｩｴﾗ ｰ｣(Tuning Space) ｳﾖｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_DVBC_TUNING_SPACE			-7119	// DirectShow, DVB-C ﾆｩｴﾗ ｰ｣(Tuning Space) ｳﾖｱ・ｽﾇﾆﾐ
#define		ERR_DX_FAILED_TO_PUT_DVBS_TUNING_SPACE			-7120	// DirectShow, DVB-S ﾆｩｴﾗ ｰ｣(Tuning Space) ｳﾖｱ・ｽﾇﾆﾐ

#define		ERR_COM_FAILED_TO_INITIALIZE				    -8000	// COM ﾃﾊｱ篳ｭ ｽﾇﾆﾐ

#define		ERR_PFUNC_NULL					-10000	// ﾇﾔｼﾎﾅﾍ ｹ｡ NULLﾀﾌｴﾙ...
#define		ERR_BUFFER_TOO_SMALL			-10001	// ｹﾛ ﾅｩｱ箍｡ ｳﾊｹｫ ﾀﾛｴﾙ
#define		ERR_ANSISTRING_TO_UNCODESTRING	-10002	// Ansi ｹｮﾀﾚｿｭﾀｻ Unicode ｹｮﾀﾚｿｭ ｺｯﾈｯ ｽﾇﾆﾐ
#define		ERR_UNCODESTRING_TO_ANSISTRING	-10003	// Unicode ｹｮﾀﾚｿｭﾀｻ Ansi ｹｮﾀﾚｿｭ ｺｯﾈｯ ｽﾇﾆﾐ
#define		ERR_INVALID_IRQL_LEVEL			-10004	// ﾀｯﾈｿﾇﾏﾁ・IRQL ｷｹｺｧ
#define		ERR_GET_ANYSEEDEVICE_RESOURCE		-10005	// ANYSEE ﾀ蠧｡ ﾀﾚｿ・ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_INVALID_CLASS_MEMBER		-10006	// ﾀｯﾈｿﾇﾏﾁ・ｾﾊﾀｺ ﾅｬｷ｡ｽｺ ｸ篁・
#define		ERR_CLASS_MEMBER_NULL			-10007	// ﾅｬｷ｡ｽｺ ｸ篁｡ NULLﾀﾓ.
#define		ERR_INVALID_VARIABLES			-10008	// ﾀｯﾈｿﾇﾏﾁ・ｾﾊﾀｺ ｺｯｼ・
#define		ERR_VARIABLES_NULL				-10009	// ｺｯｼ｡ NULLﾀﾓ.
#define		ERR_UNKNOWN_BOARD				-10010	// ｾﾋ ｼ・ｾﾂ ｺｸｵ・
#define		ERR_UNKNOWN_BOARD_ID			ERR_UNKNOWN_BOARD
#define		ERR_ANYSEEUSBD_FUNC				-10011	// ANYSEEUSBD ﾇﾔｼ・ｿﾀｷ・
#define		ERR_UNKNOWN_START_MODE			-10012	// ｸ｣ｴﾂ ｽﾃﾀﾛ ｹ貎ﾄ
#define		ERR_FAILED_TO_SET_TIMER			-10013	// ﾅｸﾀﾌｸﾓ ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_UNKNOWN_PIN_TYPE			-10014	// ｸ｣ｴﾂ ﾇﾉ ﾁｾｷ・
#define		ERR_UNKNOWN_PIN_FLAG			-10015	// ｸ｣ｴﾂ ﾇﾉ ﾇﾃｷ｡ｱﾗ
#define		ERR_UNKNOWN_CAPTURE_DEVICE		-10016	// ｸ｣ｴﾂ ﾄｸﾃﾄ ﾀ蠧｡
#define		ERR_STREAMING_INVALID_STATUS	-10017	// ﾀﾟｸﾈ ｽｺﾆｮｸｮｹﾖ ｻﾂ
#define		ERR_STREAMING_INVALID_CLOCK		-10018	// ﾀﾟｸﾈ ｽｺﾆｮｸｮｹﾖ ﾅｬｷｰ
#define		ERR_STREAMING_FAIL_TO_GET_CLOCK	-10019	// ｽｺﾆｮｸｮｹﾖ ﾅｬｷｰ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_EXCEED_BUFFER_SIZE			-10020	// ｹﾛ ﾅｩｱ・ﾃﾊｰ・
#define		ERR_PIN_OPEN					-10021	// ﾇﾉ ｿｭｱ・ｽﾇﾆﾐ
#define		ERR_PIN_CLOSE					-10022	// ﾇﾉ ｴﾝｱ・ｽﾇﾆﾐ
#define		ERR_FILTER_OPEN					-10023	// ﾇﾊﾅﾍ ｿｭｱ・ｽﾇﾆﾐ
#define		ERR_FILTER_CLOSE				-10024	// ﾇﾊﾅﾍ ｴﾝｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_CALL_FUNC			-10025	// ﾇﾔｼ・ﾈ｣ﾃ・ｽﾇﾆﾐ
#define		ERR_BUFFER_OVERFLOW				-10026	// ｹﾛ ｱ貘ﾌ ﾃﾊｰ・
#define		ERR_FAILED_TO_GET_PATH			-10027	// ｰ豺ﾎ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_FOLDER		ERR_FAILED_TO_GET_PATH
#define		ERR_UNKNOWN_CONTROL_CODE		-10028	// ｸ｣ｴﾂ ﾁｦｾ・ﾄﾚｵ・
#define		ERR_INVALID_CONTROL_CODE		ERR_UNKNOWN_CONTROL_CODE
#define		ERR_IOCTRL_PARAMS_INVALID		-10029	// IOCTRL ﾀ・ﾞｺｯｼ｡ ﾀﾟｸﾊ.
#define		ERR_IOCTRL_PARAMS_NULL			-10030	// IOCTRL ﾀ・ﾞｺｯｼ｡ NULLﾀﾓ.
#define		ERR_OCCUR_ERROR					-10031	// ｵ﨧ﾏｵﾇﾁ・ｾﾊﾀｺ ﾀﾏｹﾝﾀ・ｿﾀｷ・
#define		ERR_ANYSEEUSBD_BUSY				-10032	// ANYSEEUSBD LIBｰ｡ ｾ鋐ｲ ﾀﾏﾀｻ ﾇﾏｰ・ﾀﾖｴﾂ ｻﾂﾀﾓ.
#define		ERR_TIMEOUT						-10033	// ｽﾃｰ｣ ﾃﾊｰ・ｿﾀｷ・
#define		ERR_INVALID_LENGTH				-10034	// ﾀﾟｸﾈ ｱ貘ﾌ
#define		ERR_FAILED_TO_GET_COUNT			-10035	// ｰｳｼ・ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_EXCEED_MAXIMUM_COUNT		-10036	// ﾃﾖｴ・ｰｳｼ・ﾃﾊｰ・
#define		ERR_INVALID_INDEX				-10037	// ﾀﾟｸﾈ ｻﾎ
#define		ERR_EXCEED_LENGTH				-10038	// ｱ貘ﾌ ﾃﾊｰ・
#define		ERR_I2C_FAILED_TO_WRITE			-10039	// I2C ｱ箙ﾏ ｽﾇﾆﾐ
#define		ERR_I2C_FAILED_TO_READ			-10040	// I2C ﾀﾐｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_CHIP_ID		-10041	// Chip ID ｱｸﾇﾏｱ・ﾀﾐｱ・ ｽﾇﾆﾐ 
#define		ERR_INVALID_CHIP_ID				-10042	// ﾀﾟｸﾈ Chip ID
#define		ERR_INVALID_CLOCK_MODE			-10043	// ﾀﾟｸﾈ ﾅｬｷｰ ｹ貎ﾄ
#define		ERR_FAILED_TO_SET_PAGE			-10043	// ﾆ菎ﾌﾁ・ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_ANYSEEDEVICE_INTERRUPT_FAILURE	-10044	// ANYSEE ﾀ蠧｡ ﾀﾎﾅﾍｷｴﾆｮ ｽﾇﾆﾐ
#define		ERR_UNSUPPORTED					-10045	// ﾁﾏﾁ・ｾﾊﾀｽ.
#define		ERR_FAILED_TO_GET_DEVICE_ID		-10046	// ﾀ蠧｡ ID ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_DIVIDED_BY_ZERO				-10047	// 0ﾀｸｷﾎ ｳｪｴｭｷﾁｰ・ｽﾃｵｵﾇﾏｴﾙ.
#define		ERR_EXITING						-10048	// ﾁｾｷ・ﾁﾟｿ｡ ﾀﾖｴﾙ.
#define		ERR_FAILED_TO_GET_CAPABILITIES	-10049	// ｻ鄙・ｰ｡ｴﾉ ﾀﾚｿ・ﾁ､ｺｸ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_DEVICE_CFG	-10050	// ｼｳﾁ､ｵﾈ ﾀ蠧｡ ﾈｯｰ・ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_INVALID_BOARD_MODE			-10051	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｺｸｵ・ｹ貎ﾄ 
#define		ERR_INVALID_MODE				-10052	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｹ貎ﾄ 
#define		ERR_FAILED_TO_GET_INSTANCE_ID	-10053	// ﾀﾎｽｺﾅﾏｽｺ ID ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_CURSOR_POS	-10054	// ﾄｿｼｭ ﾀｧﾄ｡ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_SET_CURSOR_POS	-10055	// ﾄｿｼｭ ﾀｧﾄ｡ ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_INVALID_ADDRESS				-10056	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｹ・
#define		ERR_INVALID_REGISTER			-10057	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｷｹﾁｺﾅﾍ
#define		ERR_BUFFER_IS_FULL				-10058	// ｹﾛｰ｡ ｰ｡ｵ・ﾃ｡ｴﾙ.
#define		ERR_ABNORMAL_TERMINATION		-10059	// ｺ､ｻ・ｰｭﾁｦ ﾁｾｷ・
#define		ERR_UNSUPPORTED_OS_VERSION		-10060	// ﾁﾇﾁ・ｾﾊｴﾂ ｿ錞ｵﾃｼｰ・ｹ・
#define		ERR_SUPPORTED_ABOVE_WIN95		-10061	// ﾀｩｵｵｿ・5ﾀﾌｻ｡ｼｭｸｸ ﾁ・
#define		ERR_UNKNOWN_CAPTURE_FILTER		-10062	// ｸ｣ｴﾂ ﾄｸﾃﾄ ﾇﾊﾅﾍ
#define		ERR_INVALID_CAPTURE_FILTER		ERR_UNKNOWN_CAPTURE_FILTER	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾄｸﾃﾄ ﾇﾊﾅﾍ
#define		ERR_INVALID_CAPTURE_FILTER_INFO	-10063	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾄｸﾃﾄ ﾇﾊﾅﾍ ﾁ､ｺｸ
#define		ERR_FAILED_TO_GET_CAPTURE_FILTER_INFO	-10064	// ﾄｸﾃﾄ ﾇﾊﾅﾍ ﾁ､ｺｸ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_INVALID_TSFILE				-10065	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) TS ﾆﾄﾀﾏ
#define		ERR_INVALID_OFFSET				-10066	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾀｧﾄ｡
#define		ERR_INVALID_TSFILE_TOTAL_TIME	-10067	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) TS ﾆﾄﾀﾏ ﾃﾑ ﾀ扈 ｽﾃｰ｣
#define		ERR_FILE_SIZE_TOO_SMALL			-10068	// ﾆﾄﾀﾏ ﾅｩｱ箍｡ ｳﾊｹｫ ﾀﾛｴﾙ
#define		ERR_UNSUPPORT_BROADCAST_SYSTEM	-10069	// ﾁﾇﾁ・ｾﾊｴﾂ ｹ貍ﾛ ｽﾃｽｺﾅﾛ
#define		ERR_NO_IMPLEMENT				-10070	// ﾄﾚｵ蟆｡ ｱｸﾇﾇｾ・ﾀﾖﾁ・ｾﾊﾀｽ.
#define		ERR_NO_CONNECTION				-10071	// ｿｬｰ・ｾｽ.
#define		ERR_UNSUPPORT_STREAM_TYPE		-10072	// ﾁﾏﾁ・ｾﾊｴﾂ ｽｺﾆｮｸｲ ﾁｾｷ・
#define		ERR_INVALID_STREAM_TYPE			-10073	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｽｺﾆｮｸｲ ﾁｾｷ・
#define		ERR_INVALID_UNIT				-10074	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｴﾜﾀｧ
#define		ERR_INVALID_TIME_STRING			-10075	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｽﾃｰ｣ ｹｮﾀﾚｿｭ
#define		ERR_FAILED_TO_READ_MAP_REGISTER		-10076	// ｸﾊ ｷｹﾁｺﾅﾍｰｪ ﾀﾐｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_WRITE_MAP_REGISTER	-10077	// ｸﾊ ｷｹﾁｺﾅﾍｰｪ ｾｲｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_READ_MAP_FIELD	-10078	// ｸﾊ ﾇﾊｵ蟆ｪ ﾀﾐｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_WRITE_MAP_FIELD	-10079	// ｸﾊ ﾇﾊｵ蟆ｪ ｾｲｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_OS_VERSION	-10080	// ｿ錞ｵﾃｼｰ・ｹ・ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_NOT_READY					-10081	// ﾁﾘｺ｡ ｵﾇｾ・ﾀﾖﾁ・ｾﾊｴﾙ.
#define		ERR_FAILED_TO_SET_MODE			-10082	// ｹ貎ﾄ ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_INVALID_STREAM_INFO			-10083	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｽｺﾆｮｸｲ ﾁ､ｺｸ
#define		ERR_INVALID_STREAMING_INFO		-10084	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｽｺﾆｮｸｮｹﾖ ﾁ､ｺｸ
#define		ERR_NOT_MATCH					-10085	// ﾀﾏﾄ｡ﾇﾏﾁ・ｾﾊｴﾂｴﾙ.
#define		ERR_FAILED_TO_GET_PIN_FLAGS		-10086	// ﾇﾉ ﾇﾃｷ｡ｱﾗ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_OPEN_HANDLE		-10087	// ﾇﾚｵ・ｿｭｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_CLOSE_HANDLE		-10088	// ﾇﾚｵ・ｴﾝｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_CREATE_HANDLE		-10089	// ﾇﾚｵ・ｻｼｺ ｽﾇﾆﾐ
#define		ERR_FAILED_TO_CREATE_CLASS		-10090	// ﾅｬｷ｡ｽｺ ｻｼｺ ｽﾇﾆﾐ
#define		ERR_FAILED_TO_DESTORY_CLASS		-10091	// ﾅｬｷ｡ｽｺ ﾆﾄｱｫ ｽﾇﾆﾐ
#define		ERR_FAILED_TO_INITIALIZE_CLASS	-10092	// ﾅｬｷ｡ｽｺ ﾃﾊｱ篳ｭ ｽﾇﾆﾐ
#define		ERR_INVALID_COMMAND				-10093	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｸ昞ﾉｾ・
#define		ERR_COMMAND_LOCKED				-10094	// ｸ昞ﾉｾ銧｡ ｰ暿､ｵﾇｾ・ﾀﾖｴﾙ..
#define		ERR_INVALID_COMMAND_ID			-10095	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｸ昞ﾉｾ・ID
#define		ERR_FAILED_TO_CREATE_PROCESS	-10096	// ﾇﾁｷﾎｼｼｽｺ ｻｼｺ ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_WINDOWS_DIRECTORY		-10097	// ｽﾃｽｺﾅﾛ ｵｺﾅ荳ｮ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_SYSTEM_DIRECTORY		-10098	// ﾀｩｵｵｿ・・ｵｺﾅ荳ｮ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_SHELL_FAILED_TO_EXECUTE				-10099	// ﾇﾁｷﾎｱﾗｷ･ ｽﾇﾇ・ｽﾇﾆﾐ.
#define		ERR_WAIT_ABANDONED						-10100	// ｴ・・ﾁﾟﾁﾔ.
#define		ERR_FAILED_TO_GET_PROCESS_EXIT_CODE		-10101	// ﾇﾁｷﾎｼｼｽｺ ﾁｾｷ皺ｪ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_INVALID_OS							-10102	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｿ錞ｵﾃｼｰ・
#define		ERR_UNSUPPORTED_OS						-10103	// ﾁﾏﾁ・ｾﾊｴﾂ ｿ錞ｵﾃｼｰ・
#define		ERR_INVALID_PROCESSOR					-10104	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾇﾁｷﾎｼｼｼｭ
#define		ERR_INVALID_PROCESSOR_ARCHITECTURE		-10105	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾇﾁｷﾎｼｼｼｭ ｱｸﾁｶ
#define		ERR_UNSUPPORTED_PROCESSOR_ARCHITECTURE	-10106	// ﾁﾏﾁ・ｾﾊｴﾂ ﾇﾁｷﾎｼｼｼｭ ｱｸﾁｶ
#define		ERR_IMPOSSIBLE					-10107	// ｹﾟｻﾇﾒ ｼ・ｾﾂ ｿﾀｷ｡ ｹﾟｻﾇﾟｴﾙ.... 
#define		ERR_EXCEED						-10108	// ｹ・ｧﾀｻ ﾃﾊｰ戓ﾏｴﾙ.
#define		ERR_RUNNING						-10109	// ｽﾇﾇ狠ﾟｿ｡ ﾀﾖｴﾙ.
#define		ERR_FAILED_TO_RESOURCE			-10110	// ﾀﾚｿｻ ｱｸﾇﾒ ｼ・ｾﾙ.
#define		ERR_INVALID_RESOURCE			-10111	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾀﾚｿ・
#define		ERR_INVALID_LIST				-10112	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｸﾏ
#define		ERR_FAILED_TO_GET_CLASS			-10113	// ﾅｬｷ｡ｽｺ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_DIRECTORY		-10114	// ﾇ・ｵｺﾅ荳ｮ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_SET_DIRECTORY		-10115	// ﾇ・ｵｺﾅ荳ｮ ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_INVALID_DIRECTORY			-10116	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｵｺﾅ荳ｮ
#define		ERR_INVALID_SIGNAL				-10117	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｽﾅﾈ｣
#define		ERR_INVALID_NUMBER				-10118	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｹ｣
#define		ERR_BUSY						-10119	// ｹﾙｻﾛ ｻﾂﾀﾌｴﾙ.
#define		ERR_FAILED_TO_READ_REG			-10120	// ｷｹﾁｺﾅﾍ ﾀﾐｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_WRITE_REG			-10121	// ｷｹﾁｺﾅﾍ ｾｲｱ・ｽﾇﾆﾐ
#define		ERR_NOTHING						-10122	// ｵ･ﾀﾌﾅﾍｰ｡ ｾﾙ.
#define		ERR_FAILED_TO_GET_DATA			-10123	// ｵ･ﾀﾌﾅﾍｰ｡ ｰ｡ﾁｮｿﾀｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_SET_DATA			-10124	// ｵ･ﾀﾌﾅﾍ ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_NOT_MATCH_ID				-10125	// IDｰ｡ ｸﾂﾁ・ｾﾊｴﾙ.
#define		ERR_IOCTRL_TIMEOUT				-10126	// I/O ﾁｦｾ・ｸ昞ﾉｾ・ﾃｳｸｮ ｽﾃｰ｣ ﾃﾊｰ・
#define		ERR_INVALID_PROPERTY_ID			-10127	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｼﾓｼｺ Id
#define		ERR_UNSUPPORT_PROPERTY_ID		-10128	// ﾁﾏﾁ・ｾﾊｴﾂ ｼﾓｼｺ Id
#define		ERR_DEVICE_BUSY					-10129	// ﾀ蠧｡ｰ｡ ｴﾙｸ･ ﾀﾏｷﾎ ｹﾙｻﾚｴﾙ.
#define		ERR_UNSUPPORT_BAUD_RATE			-10130	// ﾁﾏﾁ・ｾﾊｴﾂ Baud Rate
#define		ERR_INVALID_BAUD_RATE			-10131	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) Baud Rate
#define		ERR_DC_ADAPTER_UNPLUGED			-10132	// DC ﾀ・・ｾ鎤ﾇﾅﾍｰ｡ ｿｬｰ盞ﾇﾁ・ｾﾊｾﾒｴﾙ.
#define		ERR_SIGNAL_CABLE_OPENED			-10133	// ｽﾅﾈ｣ ﾄﾉﾀﾌｺ暲ﾌ ｿｬｰ盞ﾇﾁ・ｾﾊｾﾒｴﾙ.
#define		ERR_ACCESS_DENY_RESOURCE		-10134	// ﾀﾚｿ・ﾁ｢ｱﾙ ｺﾒｰ｡.
#define		ERR_PNP_DEVICE_STOPPED			-10135	// PnP ﾀ蠧｡ｰ｡ ﾁﾟﾁ・ｻﾂｿ｡ ﾀﾖｴﾙ. PnP ﾀ蠧｡ｸｦ ｻ鄙・ﾒ ﾁﾘｺ｡ ｵﾇﾁ・ｾﾊｾﾒｴﾙ.
#define		ERR_DEVICE_STOPPED				-10136	// ﾀ蠧｡ｰ｡ ﾁﾟﾁ・ｻﾂｿ｡ ﾀﾖｴﾙ. ﾀ蠧｡ｸｦ ｻ鄙・ﾒ ﾁﾘｺ｡ ｵﾇﾁ・ｾﾊｾﾒｴﾙ.
#define		ERR_INVALID_CAPTURE_FILTER_LIST -10137	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾄｸﾃﾄ ﾇﾊﾅﾍ ｸﾏ
#define		ERR_FAILED_TO_MAP_IOSPACE_INTO_VIRTUAL_ADDRESS	-10138	// IO ｰ｣ｿ｡ ｵ﨧ﾏｵﾈ ｹｰｸｮﾀ・ｹｦ ｰ｡ｻ・ｹﾎ ｹ霪｡ﾇﾏｴﾂｵ･(Map) ｽﾇﾆﾐ.
#define		ERR_UNSUPPORT_INTERRUPT_MODE	-10139	// ﾁﾏﾁ・ｾﾊｴﾂ ﾀﾎﾅﾍｷｴﾆｮ ｹ貎ﾄ
#define		ERR_FAILED_TO_GET_RESOURCE_LIST	-10140	// ﾀﾚｿ・ｸﾏ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_EXCEED_STEP					-10141	// ｴﾜｰ雕ｦ ﾃﾊｰ戓ﾏｴﾙ.
#define		ERR_EXCEED_PARSE_STEP			-10142	// ｹｮｹ ﾇﾘｼｮ ｴﾜｰ雕ｦ ﾃﾊｰ戓ﾏｴﾙ.
#define		ERR_FAILED_TO_CONVERT_STRING_TO_NUMBER	-10143	// ｹｮﾀﾚｿｭﾀｻ ｼﾀﾚｷﾎ ｹﾙｲﾙｱ・ｽﾇﾆﾐ.
#define		ERR_FAILED_TO_CREATE_FILE				-10144	// ﾆﾄﾀﾏ ｻｼｺ ｽﾇﾆﾐ
#define		ERR_FAILED_TO_READY_FOR_STREAM_Q		-10145	// ｽｺﾆｮｸｲ Qｵ鯊ﾌ ﾁﾘｺ｡ ｾﾈｵﾇｾ嶸ﾙ.
#define		ERR_FAILED_TO_READY_FOR_DTV_STREAM_Q	-10146	// ｾﾆｳｯｷﾎｱﾗ TVｿ・ｽｺﾆｮｸｲ Qｵ鯊ﾌ ﾁﾘｺ｡ ｾﾈｵﾇｾ嶸ﾙ.
#define		ERR_FAILED_TO_READY_FOR_ATV_STREAM_Q	-10147	// ｾﾆｳｯｷﾎｱﾗ TVｿ・ｽｺﾆｮｸｲ Qｵ鯊ﾌ ﾁﾘｺ｡ ｾﾈｵﾇｾ嶸ﾙ.
#define		ERR_INVALID_DATE_STRING			-10148	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｳｯﾂ･ ｹｮﾀﾚｿｭ
#define		ERR_INVALID_FWDATA_LENGTH		-10149	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾆﾟｿｾ・ｵ･ﾀﾌﾅﾍ ｱ貘ﾌ
#define		ERR_INVALID_ACCESS_LEVEL		-10150	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾁ｢ｱﾙ ｷｹｺｧ
#define		ERR_INVALID_ACCESS_TYPE			-10151	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾁ｢ｱﾙ ﾁｾｷ・
#define		ERR_NOT_MATCH_TOKEN				-10152	// ﾅ萋ﾁﾀﾌ ｸﾂﾁ・ｾﾊｴﾙ.
#define		ERR_FAILED_TO_PUT_ITEM			-10153	// ﾇﾗｸ・ｳﾖｱ・ｽﾇﾆﾐ
#define		ERR_NOT_MATCH_VENDOR_ID			-10154	// Vendor Idｰ｡ ﾀﾏﾄ｡ﾇﾏﾁ・ｾﾊｴﾂｴﾙ.
#define		ERR_INVALID_HANDLE				-10155	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾇﾚｵ・
#define		ERR_DEVICE_REMOVED				-10156	// ﾀ蠧｡ｰ｡ ﾁｦｰﾅｵﾇｾ嶸ﾙ.
#define		ERR_INVALID_DEVICE				-10157	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾀ蠧｡
#define		ERR_UNKNOWN_DEVICE	ERR_INVALID_DEVICE	// ｸ｣ｴﾂ ﾀ蠧｡
#define		ERR_INVALID_DEVICE_TYPE			-10158	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾀ蠧｡ ﾁｾｷ・
#define		ERR_UNKNOWN_DEVICE_TYPE	ERR_INVALID_DEVICE_TYPE	// ｸ｣ｴﾂ ﾀ蠧｡ ﾁｾｷ・
#define		ERR_UNSUPPORTED_DEVICE			-10159	// ﾁﾏﾁ・ｾﾊｴﾂ ﾀ蠧｡
#define		ERR_UNSUPPORTED_DEVICE_TYPE		-10160	// ﾁﾏﾁ・ｾﾊｴﾂ ﾀ蠧｡ ﾁｾｷ・
#define		ERR_DEVICE_ALREADY_OPENED		-10161	// ﾀ蠧｡ｰ｡ ﾀﾌｹﾌ ｿｭｷﾁﾀﾖｴﾙ.
#define		ERR_INSUFFICIENT_RESOURCE		-10162	// ﾀﾚｿ・ｺﾎﾁｷ
#define		ERR_INVALID_ID					-10163	// ｹｫﾈｿﾇﾑ Id
#define		ERR_LOCKED						-10164	// ﾀﾌｹﾌ ｶﾌ ｰﾉｷﾁ ﾀﾖｴﾙ.
#define		ERR_UNSUPPORTED_VERSION			-10165	// ﾁﾇﾁ・ｾﾊｴﾂ ｹ・
#define		ERR_INVALID_DEVICE_ID_STRING	-10166	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾀ蠧｡ Id ｹｮﾀﾚｿｭ
#define		ERR_INVALID_GUID				-10167	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) GUID
#define		ERR_INVALID_GUID_STRING			-10168	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) GUID ｹｮﾀﾚｿｭ
#define		ERR_INVALID_CLSID				-10169	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) CLSID
#define		ERR_INVALID_CLSID_STRING		-10170	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) CLSID ｹｮﾀﾚｿｭ
#define		ERR_FAILED_TO_CONVERT_WSTRING_TO_STRING	-10171	// WCHAR ｹｮﾀﾚｿｭﾀｻ CHAR ｹｮﾀﾚｿｭｷﾎ ｺｯﾈｯ ｽﾇﾆﾐ.
#define		ERR_FAILED_TO_CONVERT_STRING_TO_WSTRING	-10172	// CHAR ｹｮﾀﾚｿｭﾀｻ WCHAR ｹｮﾀﾚｿｭｷﾎ ｺｯﾈｯ ｽﾇﾆﾐ.
#define		ERR_NO_VIDEO_WINDOW				-10173	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｺﾀ ﾀｩｵｵｿ・
#define		ERR_NO_VW						ERR_NO_VIDEO_WINDOW
#define		ERR_INVALID_VW					ERR_NO_VIDEO_WINDOW
#define		ERR_INVALID_VIDEO_WINDOW		ERR_NO_VIDEO_WINDOW
#define		ERR_NO_TV_CHANNEL_TABLE			-10174	// TV ﾃ､ｳﾎﾇ･ｰ｡ ｾｽ
#define		ERR_NO_RADIO_CHANNEL_TABLE		-10175	// ｶﾀ ﾃ､ｳﾎﾇ･ｰ｡ ｾｽ
#define		ERR_INVALID_STATE				-10176	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｻﾂ
#define		ERR_NO_STOP_STATE				-10177	// ﾁﾟﾁ・ｻﾂｰ｡ ｾﾆｴﾏｴﾙ.
#define		ERR_NO_RUN_STATE				-10178	// ｽﾇﾇ・ｻﾂｰ｡ ｾﾆｴﾏｴﾙ.
#define		ERR_RESOURCE_OPENED				-10179	// ﾀﾚｿﾌ ﾀﾌｹﾌ ｿｭｷﾈｴﾙ.
#define		ERR_NOT_LINKED					-10180	// ｿｬｰ眤ﾌ ｾﾈｵﾇｾ・ﾀﾖﾀｽ.
#define		ERR_INVALID_TYPE				-10181	// ﾀﾟｸﾈ(ｹｫﾈｿﾇﾑ) ﾁｾｷ・
#define		ERR_OPENED						-10182	// ﾀﾌｹﾌ ｿｭｷﾁ ﾀﾖｴﾙ.
#define		ERR_CLOSED						-10183	// ｴﾝﾇﾖｴﾙ.
#define		ERR_NO_CLASS					-10184	// ﾀﾎﾅﾍﾆ菎ﾌｽｺ ﾅｬｷ｡ｽｺｰ｡ ｾﾙ.
#define		ERR_INVALID_CLASS				ERR_NO_CLASS
#define		ERR_INVALID_CLASS_INTERFACE		ERR_NO_CLASS
#define		ERR_NO_CLASS_INTERFACE			ERR_NO_CLASS
#define		ERR_INVALID_READ_LENGTH			-10185	// ﾀﾟｸﾈ(ｹｫﾈｿﾇﾑ) ﾀﾐｱ・ｱ貘ﾌ
#define		ERR_INVALID_WRITE_LENGTH		-10186	// ﾀﾟｸﾈ(ｹｫﾈｿﾇﾑ) ｾｲｱ・ｱ貘ﾌ
#define		ERR_INVALID_ADDRESS_LENGTH		-10187	// ﾀﾟｸﾈ(ｹｫﾈｿﾇﾑ) ｹ・ｱ貘ﾌ
#define		ERR_INVALID_DATA_LENGTH			-10188	// ﾀﾟｸﾈ(ｹｫﾈｿﾇﾑ) ｵ･ﾀﾌﾅﾍ ｱ貘ﾌ
#define		ERR_FAILED_TO_GET_CAPTURE_DEVICE	-10189	// ﾄｸﾃﾄ ﾀ蠧｡ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_INVALID_CAPTURE_DEVICE		-10190	// ｹｫﾈｿﾇﾑ ﾄｸﾃﾄ ﾀ蠧｡
#define		ERR_CAPTURE_DEVICE_NULL			ERR_INVALID_CAPTURE_DEVICE	
#define		ERR_FAILED_TO_GET_PROCESS_LIST	-10191	// ﾇﾁｷﾎｼｼｽｺ ｸﾏ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_SET_READ_OFFSET	-10192	// ﾀﾐｱ・ﾀｧﾄ｡ ｼｳﾁ､ ｺｯｰ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_SET_WRITE_OFFSET	-10193	// ｾｲｱ・ﾀｧﾄ｡ ｼｳﾁ､ ｺｯｰ・ｽﾇﾆﾐ
#define		ERR_WAIT_ALERTED				-10194	// ｴ・・ｰ貅ｸ ｿﾀｷ・ｹﾟｻ.
#define		ERR_WAIT_USER_APC				-10195	// ｴ・・USER_APC ｿﾀｷ・ｹﾟｻ.
#define		ERR_WAIT_FAILURE				-10196	// ｴ・・ｽﾇﾆﾐ
#define		ERR_INVALID_KMUTEX				-10197	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾄｿｳﾎ ｹﾂﾅﾘｽｺ
#define		ERR_KMUTEX_LOCK_FAILURE			-10198	// ﾄｿｳﾎ ｹﾂﾅﾘｽｺ ｶ・ｰﾉｱ・ｽﾇﾆﾐ
#define		ERR_KMUTEX_UNLOCK_FAILURE		-10199	// ﾄｿｳﾎ ｹﾂﾅﾘｽｺ ｶ・ﾇｮｱ・ｽﾇﾆﾐ
#define		ERR_DISABLED					-10200	// ｻ鄙・ｱﾝﾁ・ｻﾂﾀﾌｴﾙ.
#define		ERR_STREAMQ_IS_DISABLED			-10201	// ｽｺﾆｮｸｲ Qｰ｡ ｻ鄙・ｱﾝﾁ・ｻﾂﾀﾌｴﾙ.
#define		ERR_ENABLED						-10202	// ﾀﾌｹﾌ ｻ鄙・ｻﾂﾀﾌｴﾙ.
#define		ERR_AMTWORKER_STARTED			-10203	// ﾀﾌﾀ・｡ ｽﾃﾀﾛﾇﾑ AMT ﾀﾛｾﾚｰ｡ ｵｿﾀﾛ ﾁﾟﾀﾌｴﾙ.
#define		ERR_FORCE_TO_STOP				-10204	// ｰｭﾁｦｷﾎ ﾁﾟﾁﾃﾅｰｴﾙ.
#define		ERR_ADDED_ALREADY				-10205	// ﾀﾌｹﾌ ﾃﾟｰ｡ｵﾇｾ・ﾀﾖｴﾙ.
#define		ERR_CUTOFF_DATA					-10206	// ｵ･ﾀﾌﾅﾍｰ｡ ｲ錝ｳｴﾙ.
#define		ERR_INVALID_LOCK_LEVEL			-10207	// ﾀｯﾈｿﾇﾏﾁ・LOCK ｷｹｺｧ
#define		ERR_INVALID_DATA				-10208	// ﾀﾟｸﾈ(ｹｫﾈｿﾇﾑ) ｵ･ﾀﾌﾅﾍ
#define		ERR_FAILED_TO_ENUMERATE_PROCESS	-10209	// ﾇﾁｷﾎｼｼｽｺ ｳｪｿｭ ｽﾇﾆﾐ
#define		ERR_FAILED_TO_CREATE_TOOLHELP32SNAPSHOT		-10210	// ToolHelp32 Snapshot ｻｼｺ ｽﾇﾆﾐ.
#define		ERR_FAILED_TO_GET_PROCESS32FIRST	-10211	// ﾇﾁｷﾎｼｼｽｺ32 ｸﾏﾀﾇ ﾃｹｹｰ ﾀｧﾄ｡ﾀｻ ｱｸﾇﾏｴﾂｵ･ ｽﾇﾆﾐﾇﾟｴﾙ.
#define		ERR_SCCMD_FILTERING				-10212	// ｽｺｸｶﾆｮﾄｫｵ・ｸ昞ﾉｾ・ﾇﾊﾅﾍｸｵ ｿﾀｷ・
#define		ERR_FWCOMMAND_TIMEOUT			-10213	// ﾆﾟｿｾ・ｸ昞ﾉｾ・ｽﾃｰ｣ ﾃﾊｰ・
#define		ERR_PENDING						-10214	// ﾆ豬ﾇｾ嶸ﾙ.
#define		ERR_CLOSING_STATE				-10215	// ﾀﾛｾｻ ｴﾝｰ・ﾀﾖｴﾂ ｻﾂﾀﾌｴﾙ.
#define		ERR_KSTREAMQ_WAS_OPENED_ALREADY	-10216	// ﾄｿｳﾎ ｽｺﾆｮｸｲﾅ･ｰ｡ ﾀﾌｹﾌ ｿｭｷﾁﾀﾖｴﾂ ｻﾂﾀﾌｴﾙ.
#define		ERR_FAILED_TO_GET_NODE			-10217	// ｳ・・ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_NO_KEY						-10218	// ﾅｰｰ｡ ｾﾙ.
#define		ERR_FAILED_TO_ADD_TRAYICON		-10219	// ﾆｮｷｹﾀﾌ ｾﾆﾀﾌﾄﾜ ﾃﾟｰ｡ ｽﾇﾆﾐ
#define		ERR_FAILED_TO_DELETE_TRAYICON	-10220	// ﾆｮｷｹﾀﾌ ｾﾆﾀﾌﾄﾜ ｻ霖ｦ ｽﾇﾆﾐ
#define		ERR_FAILED_TO_MODIFY_TRAYICON	-10221	// ﾆｮｷｹﾀﾌ ｾﾆﾀﾌﾄﾜ ｼ､ ｽﾇﾆﾐ
#define		ERR_NO_TRAYICON					-10222	// ﾆｮｷｹﾀﾌ ｾﾆﾀﾌﾄﾜﾀﾌ ﾁｸﾀ酩ﾏﾁ・ｾﾊｴﾂｴﾙ. ﾁ・ ﾃﾟｰ｡ｵﾇﾁ・ｾﾊｾﾒｴﾙ.
#define		ERR_FAILED_TO_LOAD_MENU			-10223	// ｸﾞｴｺ ｷﾎｵ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_SUBMENU		-10224	// ｺﾎｸﾞｴｺ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_NO_DATA						-10225	// ｵ･ﾀﾌﾅﾍ ｾｽ.
#define		ERR_FAILED_TO_ENUMERATE_WINDOWS -10226	// ﾀｩｵｵｿ・鯊ｻ ｳｪｿｭﾇﾏｴﾂｵ･ ｽﾇﾆﾐﾇﾟｴﾙ.
#define		ERR_EMPTY						-10227	// ｺ鋿ﾖｴﾙ.
#define		ERR_INVALID_HWND				-10228	// ﾀﾟｸﾈ(ｹｫﾈｿﾇﾑ) ﾀｩｵｵｿ・ﾇﾚｵ・
#define		ERR_FAILED_TO_ADD_DEVICE		-10229	// ﾀ蠧｡ ﾃﾟｰ｡ ｽﾇﾆﾐ
#define		ERR_FAILED_TO_REMOVE_DEVICE		-10230	// ﾀ蠧｡ ﾁｦｰﾅ ｽﾇﾆﾐ
#define		ERR_NO_DEVICE					-10231	// ﾀ蠧｡ｰ｡ ｾﾙ.
#define		ERR_FAILED_TO_PARSE_DATA		-10232	// ｵ･ﾀﾌﾅﾍﾀｻ ﾇﾘｼｮﾇﾏｴﾂｵ･ ｽﾇﾆﾐﾇﾟｴﾙ.
#define		ERR_FAILED_TO_GET_DEVICE		-10233	// ﾀ蠧｡ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_MAMURI_STATE				-10234	// ｸｶｹｫｸｮ ｻﾂﾀﾌｴﾙ.
#define		ERR_CLOSE_STATE					-10235	// ｴﾝﾀｺ ｻﾂﾀﾌｴﾙ.
#define		ERR_NOT_MATCH_PARENT_ID			-10236	// ｺﾎｸ・(ﾀ蠧｡) Idｿﾍ ﾀﾏﾄ｡ﾇﾏﾁ・ｾﾊｴﾂｴﾙ.
#define		ERR_NO_TSCAPTURE_DEVICE			-10237	// TS ﾄｸﾃﾄ ﾀ蠧｡ｰ｡ ｾﾆｴﾏｴﾙ.
#define		ERR_PARSER_INVALID_STEP			-10238	// ﾆﾄｼｭ, ﾀﾟｸﾈ(ｹｫﾈｿﾇﾑ) ｴﾜｰ・
#define		ERR_PARSER_INVALID_STATE		-10239	// ﾆﾄｼｭ, ﾀﾟｸﾈ(ｹｫﾈｿﾇﾑ) ｻﾂ
#define		ERR_PARSER_INVALID_STATEMENT	-10240	// ﾆﾄｼｭ, ﾀﾟｸﾈ(ｹｫﾈｿﾇﾑ) ｹｮﾀ・
#define		ERR_FAILED_TO_INSERT_MENU		-10241	// ｸﾞｴｺ ｻﾔ ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_SPECIAL_FOLDER_PIDL	-10242	// ﾆｯｼ・ﾆ嶸・PIDL ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_PATH_FROM_IDLIST		-10243	// IDｸﾏﾀｸｷﾎｺﾎﾅﾍ ｰ豺ﾎ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_USER_NAME					-10244	// ｻ鄙・ﾚｸ・ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_POWER_STATES_IS_CHANGING			-10245	// ﾀ・・ｻﾂｸｦ ｺｯｰ貮ﾏｰ・ﾀﾖｾ・ ﾃｳｸｮﾇﾏｴﾂｵ･ ｽﾇﾆﾐﾇﾟｴﾙ.
#define		ERR_FAILED_TO_SET_KSPROPERTYSET			-10246	// KsPropertySet ｼｳﾁ､ ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_KSPROPERTYSET			-10247	// KsPropertySet ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_INITIALIZE_SPINLOCK		-10248	// ｽｺﾇﾉｶ・ﾃﾊｱ篳ｭ ｽﾇﾆﾐ
#define		ERR_FAILED_TO_INITIALIZE_MUTEXLOCK		-10249	// ｹﾂﾅﾘｽｺｶ・ﾃﾊｱ篳ｭ ｽﾇﾆﾐ
#define		ERR_NO_USB_DEVICE_OBJECT				-10250	// USB ﾀ蠧｡ ｰｴﾃｼｰ｡ ｾﾙ.
#define		ERR_INVALID_USB_PIPE_PARAMS				-10251	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) USB ﾆﾄﾀﾌﾇﾁ ｸﾅｰｳｺｯｼ・
#define		ERR_DELAYTIME_FAILURE					-10252	// ｾｲｷｹｵ・ｴ・・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_LOCK						-10253	// ｶｻ ｰﾅｴﾂｵ･ ｽﾇﾆﾐﾇﾟｴﾙ.
#define		ERR_FAILED_TO_UNLOCK					-10254	// ｶｻ ﾇﾘﾁｦﾇﾏｴﾂｵ･ ｽﾇﾆﾐﾇﾟｴﾙ.
#define		ERR_AMTBD_OPEN_FAILURE					-10255	// AMTBD Libｸｦ ｿｩｴﾂｵ･ ｽﾇﾆﾐﾇﾟｴﾙ.
#define		ERR_AMTBD_CLOSED						-10256	// AMTBD Libｰ｡ ｴﾝﾇ・ﾀﾖﾀｽ.
#define		ERR_INVALID_HEADER						-10257	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ﾇ・・
#define		ERR_OPEN_FAILURE						-10258	// ｿｭｱ・ｽﾇﾆﾐ
#define		ERR_AMTLOGQ_OPEN_FAILURE				-10259	// AMTLOG-Q ｿｭｱ・ｽﾇﾆﾐ
#define		ERR_NO_AMTIOLOG							-10260	// AMTIOLogｰ｡ ｾﾙ.
#define		ERR_INVALID_BUFFER_SIZE					-10261	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) ｹﾛ ﾅｩｱ・
#define		ERR_IOSERVICE_OPEN_FAILURE				-10262	// IO ｼｭｺｺ ｿｭｱ・ｽﾇﾆﾐ
#define		ERR_IOSERVICE_CLOSE_FAILURE				-10263	// IO ｼｭｺｺ ｴﾝｱ・ｽﾇﾆﾐ
#define		ERR_CLOSE_FAILURE						-10264	// ｴﾝｱ・ｽﾇﾆﾐ
#define		ERR_OSX_IOCONNECT_FAILURE				-10265	// Apple OSX Ioconnect ｽﾇﾆﾐ
#define		ERR_UNSUPPORTED_MODE					-10266	// ﾁﾏﾁ・ｾﾊｴﾂ ｹ貎ﾄﾀﾌｴﾙ.
#define		ERR_FAILED_TO_ALLOCATE_STREAM_POINTER	-10267	// ｽｺﾆｮｸｲ ﾆﾎﾅﾍｸｦ ﾇﾒｴ郢ﾞｴﾂｵ･ ｽﾇﾆﾐﾇﾟｴﾙ.
#define		ERR_FAILED_TO_RELEASE_STREAM_POINTER	-10268	// ｽｺﾆｮｸｲ ﾆﾎﾅﾍｸｦ ﾇﾘﾁｦﾇﾏｴﾂｵ･ ｽﾇﾆﾐﾇﾟｴﾙ.
#define		ERR_STREAM_POINTER_IS_REMAINED			-10269	// ｽｺﾆｮｸｲ ﾆﾎﾅﾍｰ｡ ｳｲｾﾆ ﾀﾖｴﾙ.
#define		ERR_NO_AMTIOSTREAM_HEADER				-10270	// AMTIO ｽｺﾆｮｸｲ ﾇ・｡ ｾﾙ.
#define		ERR_FAILED_TO_SET_AMTIOSTREAM_HEADER	-10271	// AMTIO ｽｺﾆｮｸｲ ﾇ・ｦ ｼｳﾁ､ﾇﾏｴﾂｵ･ ｽﾇﾆﾐﾇﾟｴﾙ.
#define		ERR_FAILED_TO_OPEN_STREAMQ				-10272	// ｽｺﾆｮｸｲ ﾅ･ｸｦ ｿｩｴﾂｵ･ ｽﾇﾆﾐﾇﾟｴﾙ.
#define		ERR_FAILED_TO_CLOSE_STREAMQ				-10273	// ｽｺﾆｮｸｲ ﾅ･ｸｦ ｴﾝｴﾂｵ･ ｽﾇﾆﾐﾇﾟｴﾙ.
#define		ERR_FAILED_TO_ALLOCATE_IODATAQ_NOTIFICATION_PORT	-10274	// 'IODataQueueAllocateNotificationPort()' ｽﾇﾆﾐ
#define		ERR_FAILED_TO_SET_IODATAQ_NOTIFICATION_PORT			-10275	// 'IOConnectSetNotificationPort()' ｽﾇﾆﾐ
#define		ERR_IOCONNECT_MAP_MEMORY_FAILURE		-10276	// 'IOConnectMapMemory()' ｽﾇﾆﾐ			
#define		ERR_IOCONNECT_UNMAP_MEMORY_FAILURE		-10277	// 'IOConnectUnmapMemory()' ｽﾇﾆﾐ			
#define		ERR_INVALID_IODATAQ_MAPPED_MEMORY		-10278	// ｹｫﾈｿﾇﾑ(ﾀﾟｸﾈ) IODataQ ｸﾊ ｸﾞｸｮ
#define		ERR_FAILED_TO_SEND_STREAM_HEADER_FLAGS  -10279	// ｽｺﾆｮｸｲ ﾇ・・ﾇﾃｷ｡ｱﾗ ｺｸｳｻｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_THE_HOST_NAME			-10280	// ﾈ｣ｽｺﾆｮｸ・ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_FAILED_TO_GET_THE_HOST_IP			-10281	// ﾈ｣ｽｺﾆｮ IP ｱｸﾇﾏｱ・ｽﾇﾆﾐ
#define		ERR_NO_HOST_ADDRESS_LIST				-10282	// ﾈ｣ｽｺﾆｮ ｹ・ｸﾏﾀﾌ ｾﾙ.
#define		ERR_FAILED_TO_GET_THE_HOST_INFORMATION	-10283	// ﾈ｣ｽｺﾆｮ ﾁ､ｺｸ ｱｸﾇﾏｱ・ｽﾇﾆﾐ



#define		ERR_SMC_LENGTH				-170    // ｽｺｸｶﾆｮ ﾄｫｵ・ｵ･ﾀﾌﾅﾍ ｱ貘ﾌｰ｡ ﾀﾟｸﾊ.
#define		ERR_SMC_ATR_INT				-172	// ATR ｿﾀｷ・
#define		ERR_SMC_ATR_TS				-173	// ﾀﾟｸﾈ ATR TS ｹﾙﾀﾌﾆｮ
#define		ERR_SMC_NO_CARD				-174	// ｽｺｸｶﾆｮ ﾄｫｵ・ｾｽ. 
#define		ERR_SMC_INCORRECT_SW		-176	// ﾀﾟｸﾈ ｻﾂ ｿ・
#define		ERR_SMC_TIMEOUT				-179	// SMC ｽﾃｰ｣ ﾃﾊｰ・
#define		ERR_SMC_BWT_TIMEOUT			-180	// SMC WWT/BWT ｽﾃｰ｣ ﾃﾊｰ・


#define	 ERR_GETFAILED_STATUS	(DWORD)0x80000000   // ｻﾂ ｱｸﾇﾏｱ・ｽﾇﾆﾐ



#endif		// #ifndef		__ANYSEE_ERROR_H__
