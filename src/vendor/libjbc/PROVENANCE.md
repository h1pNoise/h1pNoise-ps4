# libjbc source

Source: https://github.com/sleirsgoevy/ps4-libjbc (master, retrieved 2026-10-01).
Used locally for an experimental installer access test on firmware 13.50.
The upstream repository describes firmware-independent sandbox access;
this does not establish validation on 13.50.

Local changes add a kernel-call availability probe, process identity and
pointer checks, bounded process traversal, explicit variadic arguments,
private syscall symbol names, and checks for failed directory opens.
The resolver accepts prison pointers in either kernel heap or kernel data,
as the upstream reference-count code already does. Vnodes, processes and
file descriptors still require kernel heap pointers. Resolution failures
report a stage number without recording kernel addresses.
An auth-only setter writes the existing auth-info fields after validating
the current process. It leaves UID, prison, roots and vnode references alone.
The submitter uses ShellCore auth ID and the system capability temporarily,
as described at https://flatz.github.io/, and restores the saved auth info
after task registration/start, including failures.
Only the credential and kernel access sources are used.
Credential-only changes skip replacement directory descriptors when the
saved directory vnodes are unchanged. Storage queries use this path to
temporarily change the prison and credentials without changing any roots.

The upstream repository does not provide a licence file. The project's
GPL-3.0 licence does not establish an upstream licence for these files;
their origin and local modifications are recorded separately here.
