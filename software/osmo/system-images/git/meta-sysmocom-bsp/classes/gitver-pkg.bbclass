# gitver-pkg.bbclass 
#
# Based on gitpkgv.bbclass from meta-openembedded

PKGGITH = "${@get_pkg_gith(d, '${PN}')}"
PKGGITN = "${@get_pkg_gitn(d, '${PN}')}"
PKGGITV = "${@get_pkg_gitv(d, '${PN}')}"

def gitpkgv_drop_tag_prefix(version):
    import re
    if re.match("v\d", version):
        return version[1:]
    else:
        return version

def get_pkg_gitv(d, pn):
    import os
    import bb
    from pipes import quote

    src_uri = d.getVar('SRC_URI', 1).split()
    fetcher = bb.fetch2.Fetch(src_uri, d)
    ud = fetcher.ud

    ver = "0.0-0"

    for url in ud.values():
        if url.type == 'git' or url.type == 'gitsm':
            for name, rev in url.revisions.items():
                if not os.path.exists(url.localpath):
                    return None

                vars = { 'repodir' : quote(url.localpath),
                         'rev' : quote(rev) }

                # Verify of the hash is present 
                try:
                    bb.fetch2.runfetchcmd(
                        "cd %(repodir)s && "
                        "git describe %(rev)s --always 2>/dev/null" % vars,
                        d, quiet=True).strip()
                    
                except Exception:
                    bb.fetch2.runfetchcmd(
                        "cd %(repodir)s && git fetch 2>/dev/null" % vars,
                        d, quiet=True).strip()

                # Try to get a version using git describe
                try:
                    output = bb.fetch2.runfetchcmd(
                        "cd %(repodir)s && "
                        "git describe %(rev)s --long 2>/dev/null" % vars,
                        d, quiet=True).strip()

                    ver = gitpkgv_drop_tag_prefix(output)

                except Exception:
                    try:
                        commits = bb.fetch2.runfetchcmd(
                            "cd %(repodir)s && "
                            "git rev-list %(rev)s --count 2> /dev/null " % vars,
                            d, quiet=True).strip()

                        if commits == "":
                           commits = "0"

                        rev = bb.fetch2.get_srcrev(d).split('+')[1]
                    
                        ver = "0.0-%s-g%s" % (commits, rev[:7])

                    except Exception:
                        pass
    return ver

def get_pkg_gitn(d, pn):
    import os
    import bb
    from pipes import quote

    src_uri = d.getVar('SRC_URI', 1).split()
    fetcher = bb.fetch2.Fetch(src_uri, d)
    ud = fetcher.ud

    for url in ud.values():
        if url.type == 'git' or url.type == 'gitsm':
            for name, rev in url.revisions.items():
                if not os.path.exists(url.localpath):
                    return None

                vars = { 'repodir' : quote(url.localpath),
                         'rev' : quote(rev) }

                # Verify of the hash is present 
                try:
                    bb.fetch2.runfetchcmd(
                        "cd %(repodir)s && "
                        "git describe %(rev)s --always 2>/dev/null" % vars,
                        d, quiet=True).strip()
                    
                except Exception:
                    bb.fetch2.runfetchcmd(
                        "cd %(repodir)s && git fetch 2>/dev/null" % vars,
                        d, quiet=True).strip()


                try:
                    tag = bb.fetch2.runfetchcmd(
                        "cd %(repodir)s && "
                        "git describe --abbrev=0 %(rev)s 2>/dev/null" % vars,
                        d, quiet=True).strip()
 
                    vars = { 'repodir' : quote(url.localpath),
                             'rev' : quote(rev),
                             'tag' : quote(tag) }

                    commits = bb.fetch2.runfetchcmd(
                        "cd %(repodir)s && "
                        "git rev-list %(rev)s ^%(tag)s --count 2> /dev/null " % vars,
                        d, quiet=True).strip()

                    return commits

                except Exception:
                    commits = bb.fetch2.runfetchcmd(
                        "cd %(repodir)s && "
                        "git rev-list %(rev)s --count 2> /dev/null " % vars,
                        d, quiet=True).strip()

                    if commits == "":
                        commits = "0"

                    return commits

    return '0'


def get_pkg_gith(d, pn):
    import os
    import bb
    from pipes import quote

    src_uri = d.getVar('SRC_URI', 1).split()
    fetcher = bb.fetch2.Fetch(src_uri, d)
    ud = fetcher.ud

    for url in ud.values():
        if url.type == 'git' or url.type == 'gitsm':
            for name, rev in url.revisions.items():
                if not os.path.exists(url.localpath):
                    return None
                else:
                    return rev

    return None

