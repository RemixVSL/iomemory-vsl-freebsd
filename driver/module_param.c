
//-----------------------------------------------------------------------------
// Copyright (c) 2011-2014, Fusion-io, Inc.(acquired by SanDisk Corp. 2014)
// Copyright (c) 2014 SanDisk Corp. and/or all its affiliates. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of the SanDisk Corp. nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
// OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------
#if !defined (__FreeBSD__)
#error This file supports FreeBSD only
#endif

#include "port-internal.h"
#include <sys/sysctl.h>

#include "fio/port/kfio.h"

/**
 * sysctl tunables
 */
SYSCTL_NODE(_hw, OID_AUTO, fio, CTLFLAG_RD, 0, "fio driver parameters");

TUNABLE_INT("hw.fio.strict_sync", &strict_sync);
SYSCTL_INT(_hw_fio, OID_AUTO, strict_sync, CTLFLAG_RW, &strict_sync, -1, "Force strict write flushing on early non-powercut safe cards. (1=enable, 0=disable, -1=auto) Do not change for newer cards.");
TUNABLE_INT("hw.fio.disable_msix", &disable_msix);
SYSCTL_INT(_hw_fio, OID_AUTO, disable_msix, CTLFLAG_RW, &disable_msix, 0, "N/A");
TUNABLE_INT("hw.fio.bypass_ecc", &bypass_ecc);
SYSCTL_INT(_hw_fio, OID_AUTO, bypass_ecc, CTLFLAG_RW, &bypass_ecc, 0, "N/A");
TUNABLE_INT("hw.fio.groomer_low_water_delta_hpcnt", &groomer_low_water_delta_hpcnt);
SYSCTL_INT(_hw_fio, OID_AUTO, groomer_low_water_delta_hpcnt, CTLFLAG_RW, &groomer_low_water_delta_hpcnt, 1, "The proportion of logical space free over the runway that represents 'the wall'");
TUNABLE_INT("hw.fio.disable_scanner", &disable_scanner);
SYSCTL_INT(_hw_fio, OID_AUTO, disable_scanner, CTLFLAG_RW, &disable_scanner, 0, "For use only under the direction of Customer Support.");
TUNABLE_INT("hw.fio.use_command_timeouts", &use_command_timeouts);
SYSCTL_INT(_hw_fio, OID_AUTO, use_command_timeouts, CTLFLAG_RW, &use_command_timeouts, 1, "Use the command timeout registers");
TUNABLE_INT("hw.fio.use_large_pcie_rx_buffer", &use_large_pcie_rx_buffer);
SYSCTL_INT(_hw_fio, OID_AUTO, use_large_pcie_rx_buffer, CTLFLAG_RW, &use_large_pcie_rx_buffer, 0, "If true, use 1024 byte PCIe rx buffer. This improves performance but causes NMIs on some specific hardware.");
TUNABLE_INT("hw.fio.rsort_memory_limit_MiB", &rsort_memory_limit_MiB);
SYSCTL_INT(_hw_fio, OID_AUTO, rsort_memory_limit_MiB, CTLFLAG_RW, &rsort_memory_limit_MiB, 0, "Memory limit in MiBytes for rsort rescan.");
TUNABLE_INT("hw.fio.capacity_warning_threshold", &capacity_warning_threshold);
SYSCTL_INT(_hw_fio, OID_AUTO, capacity_warning_threshold, CTLFLAG_RW, &capacity_warning_threshold, 1000, "If the reserve space is below this threshold (in hundredths of percent), warnings will be issued.");
TUNABLE_INT("hw.fio.enable_unmap", &enable_unmap);
SYSCTL_INT(_hw_fio, OID_AUTO, enable_unmap, CTLFLAG_RW, &enable_unmap, 0, "Enable UNMAP support.");
TUNABLE_INT("hw.fio.iodrive_load_eb_map", &iodrive_load_eb_map);
SYSCTL_INT(_hw_fio, OID_AUTO, iodrive_load_eb_map, CTLFLAG_RW, &iodrive_load_eb_map, 1, "For use only under the direction of Customer Support.");
TUNABLE_INT("hw.fio.use_new_io_sched", &use_new_io_sched);
SYSCTL_INT(_hw_fio, OID_AUTO, use_new_io_sched, CTLFLAG_RW, &use_new_io_sched, 1, "N/A");
TUNABLE_INT("hw.fio.groomer_high_water_delta_hpcnt", &groomer_high_water_delta_hpcnt);
SYSCTL_INT(_hw_fio, OID_AUTO, groomer_high_water_delta_hpcnt, CTLFLAG_RW, &groomer_high_water_delta_hpcnt, 160, "The proportion of logical space over the low watermark where grooming starts (in ten-thousandths)");
TUNABLE_INT("hw.fio.preallocate_mb", &preallocate_mb);
SYSCTL_INT(_hw_fio, OID_AUTO, preallocate_mb, CTLFLAG_RW, &preallocate_mb, 0, "The megabyte limit for FIO_PREALLOCATE_MEMORY. This will prevent the driver from potentially using all of the system's non-paged memory.");
TUNABLE_STR("hw.fio.exclude_devices", exclude_devices, sizeof(exclude_devices));
SYSCTL_STRING(_hw_fio, OID_AUTO, exclude_devices, CTLFLAG_RW, exclude_devices,
              sizeof(exclude_devices), "List of cards to exclude from driver initialization (comma separated list of <domain>:<bus>:<slot>.<func>)");
TUNABLE_INT("hw.fio.parallel_attach", &parallel_attach);
SYSCTL_INT(_hw_fio, OID_AUTO, parallel_attach, CTLFLAG_RW, &parallel_attach, 1, "For use only under the direction of Customer Support.");
TUNABLE_INT("hw.fio.enable_ecc", &enable_ecc);
SYSCTL_INT(_hw_fio, OID_AUTO, enable_ecc, CTLFLAG_RW, &enable_ecc, 1, "N/A");
TUNABLE_INT("hw.fio.rmap_memory_limit_MiB", &rmap_memory_limit_MiB);
SYSCTL_INT(_hw_fio, OID_AUTO, rmap_memory_limit_MiB, CTLFLAG_RW, &rmap_memory_limit_MiB, 3100, "Memory limit in MiBytes for rmap rescan.");
TUNABLE_INT("hw.fio.expected_io_size", &expected_io_size);
SYSCTL_INT(_hw_fio, OID_AUTO, expected_io_size, CTLFLAG_RW, &expected_io_size, 0, "Timeout for data log compaction while shutting down.");
TUNABLE_INT("hw.fio.tintr_hw_wait", &tintr_hw_wait);
SYSCTL_INT(_hw_fio, OID_AUTO, tintr_hw_wait, CTLFLAG_RW, &tintr_hw_wait, 0, "N/A");
TUNABLE_INT("hw.fio.iodrive_load_midprom", &iodrive_load_midprom);
SYSCTL_INT(_hw_fio, OID_AUTO, iodrive_load_midprom, CTLFLAG_RW, &iodrive_load_midprom, 1, "Load the midprom");
TUNABLE_INT("hw.fio.auto_attach_cache", &auto_attach_cache);
SYSCTL_INT(_hw_fio, OID_AUTO, auto_attach_cache, CTLFLAG_RW, &auto_attach_cache, 1, "Controls directCache behavior after an unclean shutdown: 0 = disable (cache is discarded and manual rebinding is necessary), 1 = enable (default).");
TUNABLE_INT("hw.fio.use_modules", &use_modules);
SYSCTL_INT(_hw_fio, OID_AUTO, use_modules, CTLFLAG_RW, &use_modules, -1, "Number of NAND modules to use");
TUNABLE_INT("hw.fio.make_assert_nonfatal", &make_assert_nonfatal);
SYSCTL_INT(_hw_fio, OID_AUTO, make_assert_nonfatal, CTLFLAG_RW, &make_assert_nonfatal, 0, "For use only under the direction of Customer Support.");
TUNABLE_INT("hw.fio.enable_discard", &enable_discard);
SYSCTL_INT(_hw_fio, OID_AUTO, enable_discard, CTLFLAG_RW, &enable_discard, 1, "For use only under the direction of Customer Support.");
TUNABLE_INT("hw.fio.max_md_blocks_per_device", &max_md_blocks_per_device);
SYSCTL_INT(_hw_fio, OID_AUTO, max_md_blocks_per_device, CTLFLAG_RW, &max_md_blocks_per_device, 0, "For use only under the direction of Customer Support.");
TUNABLE_INT("hw.fio.force_soft_ecc", &force_soft_ecc);
SYSCTL_INT(_hw_fio, OID_AUTO, force_soft_ecc, CTLFLAG_RW, &force_soft_ecc, 0, "Forces software ECC in all cases");
TUNABLE_INT("hw.fio.max_requests", &max_requests);
SYSCTL_INT(_hw_fio, OID_AUTO, max_requests, CTLFLAG_RW, &max_requests, 5000, "How many requests pending in iodrive");
TUNABLE_INT("hw.fio.global_slot_power_limit_mw", &global_slot_power_limit_mw);
SYSCTL_INT(_hw_fio, OID_AUTO, global_slot_power_limit_mw, CTLFLAG_RW, &global_slot_power_limit_mw, 24750, "Global PCIe slot power limit in milliwatts. Performance will be throttled to not exceed this limit in any PCIe slot.");
TUNABLE_INT("hw.fio.read_pipe_depth", &read_pipe_depth);
SYSCTL_INT(_hw_fio, OID_AUTO, read_pipe_depth, CTLFLAG_RW, &read_pipe_depth, 32, "Max number of read requests outstanding in hardware.");
TUNABLE_INT("hw.fio.force_minimal_mode", &force_minimal_mode);
SYSCTL_INT(_hw_fio, OID_AUTO, force_minimal_mode, CTLFLAG_RW, &force_minimal_mode, 0, "N/A");
TUNABLE_INT("hw.fio.auto_attach", &auto_attach);
SYSCTL_INT(_hw_fio, OID_AUTO, auto_attach, CTLFLAG_RW, &auto_attach, 1, "Automatically attach drive during driver initialization: 0 = disable attach, 1 = enable attach (default). Note for Windows only: The driver will only attach if there was a clean shutdown, otherwise the fiochkdrv utility will perform the full scan attach, 2 = Windows only: Forces the driver to do a full rescan (if needed).");
TUNABLE_STR("hw.fio.external_power_override", external_power_override, sizeof(external_power_override));
SYSCTL_STRING(_hw_fio, OID_AUTO, external_power_override, CTLFLAG_RW, external_power_override,
              sizeof(external_power_override), "Override external power requirement on boards that normally require it. (comma-separated list of adapter serial numbers)");
TUNABLE_INT("hw.fio.compaction_timeout_ms", &compaction_timeout_ms);
SYSCTL_INT(_hw_fio, OID_AUTO, compaction_timeout_ms, CTLFLAG_RW, &compaction_timeout_ms, 600000, "Timeout in ms for data log compaction while shutting down.");
TUNABLE_INT("hw.fio.disable_groomer", &disable_groomer);
SYSCTL_INT(_hw_fio, OID_AUTO, disable_groomer, CTLFLAG_RW, &disable_groomer, 0, "For use only under the direction of Customer Support.");
TUNABLE_STR("hw.fio.include_devices", include_devices, sizeof(include_devices));
SYSCTL_STRING(_hw_fio, OID_AUTO, include_devices, CTLFLAG_RW, include_devices,
              sizeof(include_devices), "Whitelist of cards to include in driver initialization (comma separated list of <domain>:<bus>:<slot>.<func>)");
TUNABLE_STR("hw.fio.preallocate_memory", preallocate_memory, sizeof(preallocate_memory));
SYSCTL_STRING(_hw_fio, OID_AUTO, preallocate_memory, CTLFLAG_RW, preallocate_memory,
              sizeof(preallocate_memory), "Causes the driver to pre-allocate the RAM it needs");
TUNABLE_INT("hw.fio.disable_msi", &disable_msi);
SYSCTL_INT(_hw_fio, OID_AUTO, disable_msi, CTLFLAG_RW, &disable_msi, 0, "N/A");
TUNABLE_INT("hw.fio.scsi_queue_depth", &scsi_queue_depth);
SYSCTL_INT(_hw_fio, OID_AUTO, scsi_queue_depth, CTLFLAG_RW, &scsi_queue_depth, 256, "The queue depth that is advertised to the OS SCSI interface.");
