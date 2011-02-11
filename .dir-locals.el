;;; This file is used by Emacs to configure it's code editing modes to correctly
;;; indent cbootimage style source code.

((nil . ((indent-tabs-mode . t)
         (tab-width . 4)
         (fill-column . 80)))
 (c-mode . ((c-tab-always-indent . nil)
            (c-basic-offset . 4)))
 (c++-mode . ((c-tab-always-indent . nil)
              (c-basic-offset . 4))))
