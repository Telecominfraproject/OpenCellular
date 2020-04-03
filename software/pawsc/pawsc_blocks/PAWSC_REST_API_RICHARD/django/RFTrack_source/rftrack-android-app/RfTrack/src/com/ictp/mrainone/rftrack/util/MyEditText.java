// see: http://www.pcsalt.com/android/create-alertdialog-with-complex-custom-layout-programmatically/

package com.ictp.mrainone.rftrack.util;

import android.content.Context;
import android.view.ViewGroup.LayoutParams;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.LinearLayout;

public class MyEditText extends EditText {

	public MyEditText(Context context) {
		super(context);
		this.setSingleLine();
		this.setImeOptions(EditorInfo.IME_ACTION_DONE);
		this.setImeActionLabel("Done", EditorInfo.IME_ACTION_DONE);
		this.setLayoutParams(new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
	}
}
