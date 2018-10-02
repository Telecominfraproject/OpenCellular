# gitver-repo.bbclass
#
# Based on gitpkgv.bbclass from meta-openembedded

REPODIR ?= "${THISDIR}"

REPOGITH  = "${@get_repo_gith(d,  '${REPODIR}')}"
REPOGITN  = "${@get_repo_gitn(d,  '${REPODIR}')}"
REPOGITV  = "${@get_repo_gitv(d,  '${REPODIR}')}"
REPOGITT  = "${@get_repo_gitt(d,  '${REPODIR}')}"
REPOGITFN = "${@get_repo_gitfn(d, '${REPODIR}', '${REPOFILE}')}"

def gitver_repo_drop_tag_prefix(version):
    import re
    if re.match("v\d", version):
        return version[1:]
    else:
        return version

def get_repo_gitv(d, repodir):
    import os
    import bb
    from pipes import quote

    vars = { 'repodir' : quote(repodir) }

    try:
        output = bb.fetch2.runfetchcmd(
            "git -C %(repodir)s describe --long 2>/dev/null" % vars, 
            d, quiet=True).strip()

        ver = gitver_repo_drop_tag_prefix(output)

    except Exception:
        return None

    return ver

def get_repo_gitn(d, repodir):
    import os
    import bb
    from pipes import quote

    vars = { 'repodir' : quote(repodir) }
    
    try:
        
        tag = bb.fetch2.runfetchcmd(
            "git -C %(repodir)s describe --abbrev=0 2>/dev/null" % vars, 
            d, quiet=False).strip()

        vars = { 'repodir' : quote(repodir),
                 'tag' : quote(tag) }

        commits = bb.fetch2.runfetchcmd(
                        "git -C %(repodir)s rev-list %(tag)s.. --count 2> /dev/null" % vars,
                        d, quiet=True).strip()

        return commits

    except Exception:
        commits = bb.fetch2.runfetchcmd(
            "git -C %(repodir)s rev-list --count HEAD 2>/dev/null" % vars, 
            d, quiet=True).strip()

        if commits == "":
            commits = "0"

        return commits

def get_repo_gitt(d, repodir):
    import os
    import bb
    from pipes import quote

    vars = { 'repodir' : quote(repodir) }

    try:
        tag = bb.fetch2.runfetchcmd(
            "git -C %(repodir)s describe --abbrev=0 2>/dev/null" % vars, 
            d, quiet=True).strip()

        return tag

    except Exception:
        return None

def get_repo_gith(d, repodir):
    import os
    import bb
    from pipes import quote

    vars = { 'repodir' : quote(repodir) }

    try:
        hash = bb.fetch2.runfetchcmd(
            "git -C %(repodir)s rev-parse HEAD 2>/dev/null" % vars, 
            d, quiet=True).strip()

        return hash
    
    except Exception:
        return None

def get_repo_gitfn(d, repodir, repofile):
    import os
    import bb
    from pipes import quote

    vars = { 'repodir'  : quote(repodir),
             'repofile' : quote(repofile) }
    
    try:
        
        tag = bb.fetch2.runfetchcmd(
            "git -C %(repodir)s describe --abbrev=0 2>/dev/null" % vars, 
            d, quiet=False).strip()

        vars = { 'repodir'  : quote(repodir),
                 'repofile' : quote(repofile),
                 'tag'      : quote(tag) }

        commits = bb.fetch2.runfetchcmd(
                        "git -C %(repodir)s rev-list --count %(tag)s.. %(repofile)s 2> /dev/null" % vars,
                        d, quiet=True).strip()

        return commits

    except Exception:
        commits = bb.fetch2.runfetchcmd(
            "git -C %(repodir)s rev-list --count HEAD %(repofile)s 2>/dev/null" % vars, 
            d, quiet=True).strip()

        if commits == "":
            commits = "0"

        return commits


