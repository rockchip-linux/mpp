# Security Policy

## Supported Versions

The following versions of MPP are currently supported with security updates:

| Version                      | Supported                      |
| :--------------------------- | :----------------------------- |
| develop branch (latest)      | ✅ Fully supported              |
| release branch (latest)      | ✅ Fully supported              |
| Older release branches       | ⚠️ Critical security fixes only |
| Archived versions (&lt; 1.0) | ❌ No longer supported          |

## Reporting a Vulnerability

**Please do not** report security vulnerabilities through public GitHub issues, as this could expose the vulnerability to malicious actors.

If you discover a security vulnerability in MPP, please report it privately using the following methods:

### Reporting Channels

1. **Primary Contact** (Recommended)
   - Email the maintainer at: **herman.chen@rock-chips.com**
   - Subject format: `[SECURITY] MPP vulnerability report - [brief description]`

2. **Alternative Channel**
   - If you cannot reach the primary maintainer, please email: rockchip-opensource@rock-chips.com
   - Or submit a private ticket via [Rockchip Redmine](http://redmine.rockchip.com.cn)

### Report Content

Please include the following information to help us quickly locate and fix the issue:

- **Vulnerability Description**: Type of vulnerability (e.g., buffer overflow, use-after-free, integer overflow) and scope of impact
- **Affected Versions**: Specific MPP version or Git commit hash
- **Affected Components**: Specific modules (e.g., codec/parser/HAL/OSAL/allocator, etc.)
- **Environment Details**:
  - Rockchip SoC model (e.g., RK3588, RK3568, etc.)
  - Operating system (Linux/Android version)
  - Kernel version
- **Reproduction Steps**: Detailed steps to reproduce or PoC (Proof of Concept) code
- **Suggested Fix** (optional): If you have a proposed fix, please include it

## Response Process

Upon receiving a security report, the maintenance team will follow this process:

| Timeframe           | Action                                                       |
| :------------------ | :----------------------------------------------------------- |
| Within 48 hours     | Acknowledge receipt and begin initial assessment             |
| Within 7 days       | Complete vulnerability verification and severity assessment, provide feedback |
| Within 30 days      | Release security patch (critical vulnerabilities may be faster) |
| After patch release | Public disclosure of vulnerability details (coordinated disclosure) |

## Disclosure Policy

- **Coordinated Disclosure**: We follow coordinated disclosure principles. After the vulnerability is fixed, details will typically be publicly disclosed within 90 days, or adjusted according to the reporter's preferences
- **Acknowledgment**: Once the vulnerability is confirmed and fixed, we will credit the reporter in the release notes (unless you wish to remain anonymous)
- **CVE**: If a CVE identifier is needed, please let us know in advance and we will assist with the application

## Security Best Practices

When using MPP, the following security measures are recommended:

1. **Input Validation**: Ensure media streams passed to MPP come from trusted sources, or perform strict validation before decoding
2. **Memory Limits**: Use `mpp_buffer_group_limit_config` to limit memory usage and prevent resource exhaustion attacks
3. **Sandboxing**: Run MPP-related processes in a privilege-restricted environment
4. **Timely Updates**: Keep MPP updated to the latest version to receive the latest security fixes
5. **Kernel Security**: Ensure the underlying kernel drivers (vcodec_service/v4l2) are updated to the latest version

## Known Security Issues

View [GitHub Security Advisories](https://github.com/HermanChen/mpp/security/advisories) for publicly disclosed security announcements.

---

Thank you for contributing to MPP security!
