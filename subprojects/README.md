# Sub-projects

Dependencies that are not packaged separately.

Every sub-project in this directory is a Git subtree and can be
updated to match upstream.

## MS-Numpress

  - Original Repository: https://github.com/ms-numpress/ms-numpress
  - Peter's Maintained Fork: https://github.com/pjones/ms-numpress
  - License: http://www.apache.org/licenses/LICENSE-2.0

To update:

```
git subtree pull --squash --prefix=subprojects/msnumpress \
  https://github.com/pjones/ms-numpress.git pjones
```
