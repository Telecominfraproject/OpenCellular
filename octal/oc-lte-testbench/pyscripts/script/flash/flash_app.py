import sys, os
# import opentest.script.testrunner as testrunner

from tools.lte import lte_software
from opentest.interface import filexfer

class FlashApp():

    # @testrunner.testcase("Copy boot by flash files", critical=True)
    def CM_CLN002(self, context):
        if context.FLASH_CLEAR_APP:
            context.logger.info('Clearing mnt/app content')
            EXCLUDE_FILES = ['ssh', '.ssh', 'env', 'ubootenv', '.sign1985', 'passwd', 'shadow']
            cmd_base = 'cd /mnt/app; find -maxdepth 1'
            for excluded_file in EXCLUDE_FILES:
                cmd_base = cmd_base + ' ! -name %s' % excluded_file
            context.server.ssh.execute_command(cmd_base + ' -exec rm -rf {} \;', assertexitcode=None)
        context.logger.info("Updating final file list...")
        lte_software.update_software(context)
        content = context.server.ssh.execute_command('ls -l /mnt/app')
        context.logger.info("Final mnt/app content: \n%s" % content)
