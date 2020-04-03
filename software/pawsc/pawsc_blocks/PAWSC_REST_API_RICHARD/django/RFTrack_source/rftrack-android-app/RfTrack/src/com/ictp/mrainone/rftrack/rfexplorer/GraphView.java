/**
 * 
 */
package com.ictp.mrainone.rftrack.rfexplorer;

/**
 */
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.view.View;

/**
 * @author Ashwin Kumar
 * 
 */
public class GraphView extends View 
{
	// tipi di grafici gestiti dalla classe
	public static boolean BAR = true;
	public static boolean LINE = false;

	private Paint paint;
	
	//----------------------------------------
	private String[] horlabels;		// label orizzontali
	private int nHorizontalDivisions;	// n. divisioni orizzontali

	private String[] verlabels;		// label verticali
	private int nVerticalDivisions; // n. divisioni verticali
	
	private String title;			// titolo
	private boolean type;			// tipo di grafico

	//----------------------------------------
	private float[] values;			// valori da visualizzare
	private int datalength;			// n. campioni da visualizzare
	private float max;				// valore max
	private float min;				// valore min
	private float diff;				// delta max - min
	
	//----------------------------------------
	// parametri colori grafico
	private int colorPoint;			// colore del punto disegnato
	private int colorText;			// colore del testo
	private int colorAxis;			// colore delle linee degli assi
	
	// dimensione testo
	// http://stackoverflow.com/questions/2025282/difference-between-px-dp-dip-and-sp-on-android
	// The TEXT SIZE expressed in dp
	// dp, Density-independent Pixels:
	// an abstract unit that is based on the physical density of the screen
	private static final float text_size = 20.0f;
	private float textHeightPx;				// font height (correspond of size) in scaled pixel
	private float maxWidthVerLabels;
	private float minWidthFree;				// minima larghezza lasciata libera
	
	//----------------------------------------
	// parametri dimensionamento grafico
	private float border;
	private float margin_X;
	private float margin_Y;
	private float horstart;
	private float height;			// altezza grafico
	private float width;			// larghezza grafico
	private float graphheight;
	private float graphwidth;
	// per il grafico a barre rappresenta la larghezza della barra,
	// per il grafico normale rappresente lo spiazzamento x per raggiungere il punto finale
	private float colwidth;
	//----------------------------------------

	// funzioni accessorie font
	// http://stackoverflow.com/questions/9743098/android-text-on-canvas
	// http://stackoverflow.com/questions/6232541/android-measuretext-return-pixels-based-on-scaled-pixels
	//
	public float TextSizeScaledPixel(float txt_size)
	{
		// vedi:
		// http://stackoverflow.com/questions/9743098/android-text-on-canvas
		// http://stackoverflow.com/questions/6232541/android-measuretext-return-pixels-based-on-scaled-pixels
		// imposta le dimensioni testo:
		// Get the screen's density scale
		final float densityMultiplier = getResources().getDisplayMetrics().density;
		// Convert the dps to pixels, based on density scale
		float SizePx = (txt_size * densityMultiplier + 0.5f);
		return SizePx;
	}
	
	//----------------------------------------

	public GraphView(Context context,
			float WidthDraw,
			float HeightDraw,
			float[] valuesGraph, 
			String title,
			String[] horlabels, 
			String[] verlabels, 
			boolean type
			) {
		super(context);
		
		if(WidthDraw < 1.0)
		{
			width = getWidth() - 1;			// width of the current drawing layer
		}
		else
		{
			width = WidthDraw;
		}
		// get height of drawing area
		if(HeightDraw < 1.0)
		{
			height = getHeight();		// height of the current drawing layer
		}
		else
		{
			height = HeightDraw;		// height of the current drawing layer
		}
		
		// assegna il titolo
		if (title == null)
			title = "";
		else
			this.title = title;
		
		// assegna i valori da visualizzare
		if (valuesGraph == null)
			values = new float[0];
		else
			this.values = valuesGraph;

		// assegna le label orizzontali
		if (horlabels == null)
			this.horlabels = new String[0];
		else
			this.horlabels = horlabels;
		
		// assegna le label verticali
		if (verlabels == null)
			this.verlabels = new String[0];
		else
			this.verlabels = verlabels;
		
		this.type = type;				// tipo di grafico
		
		
//		paint = new Paint();
// mr
		paint = new Paint(Paint.ANTI_ALIAS_FLAG);
		
		//---------------------------------------
		// vedi:
		// http://stackoverflow.com/questions/9743098/android-text-on-canvas
		// http://stackoverflow.com/questions/6232541/android-measuretext-return-pixels-based-on-scaled-pixels
		// imposta le dimensioni testo (corrisponde all'altezza del font):
		textHeightPx = TextSizeScaledPixel(text_size);
		/**
		// Get the screen's density scale
		final float densityMultiplier = getResources().getDisplayMetrics().density;
		// Convert the dps to pixels, based on density scale
		float textSizePx = (text_size * densityMultiplier + 0.5f);
		**/
		paint.setTextSize(textHeightPx);
		
		// in base alle label verticali, calcola la larghezza max da associare
		maxWidthVerLabels = 0.0f;
		for (int i = 0; i < verlabels.length; i++) 
		{
			float size = paint.measureText(verlabels[i]);
			if(maxWidthVerLabels < size)
				maxWidthVerLabels = size;
		}
		
		// calcola la minima larghezza da lasciare libera
		minWidthFree = paint.measureText("O");
		minWidthFree = minWidthFree / 4.0f;
	}

	// in base ai valori di min, max, modifica i valori di verlabels
	public void setVerLabels(float min, float max)
	{
		String result;
		float delta;
		
		if(verlabels.length <= 0)
		{
			return;
		}
		delta = (max - min);
		if(verlabels.length>1)
		{
			delta = delta / (float)(verlabels.length-1);
		}
		for (int i = 0; i < verlabels.length; i++) 
		{
			result = String.format("%.0f", max - (float)(i)*delta);
			verlabels[i] = result;
		}
	}

	// in base ai valori di min, max, modifica i valori di horlabels
	public void setHorLabels(float min, float max)
	{
		String result;
		float delta;
		
		if(horlabels.length <= 0)
		{
			return;
		}
		delta = (max - min);
		if(horlabels.length>1)
		{
			delta = delta / (float)(horlabels.length-1);
		}
		for (int i = 0; i < horlabels.length; i++) 
		{
			result = String.format("%.0f", min + (float)(i)*delta);
			horlabels[i] = result;
		}
	}
	
	// aggiorna i dati del grafico
	public void updateValues(float[] values)
	{
		// assegna i valori da visualizzare
		if (values == null)
			values = new float[0];
		else
			this.values = values;
		// forza l'aggiornamento del grafico
		invalidate();
	}
	
	// chiama invalidate per causare il refresh del grafico
	public void refreshGraph() 
	{
		// mPath.reset();
		invalidate();
	}
	
	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) 
	{
		super.onMeasure(widthMeasureSpec, heightMeasureSpec);
		this.setMeasuredDimension((int)width, (int)height);
	}
	
	private void SetValuesParameters()
	{
		datalength = values.length;		// n. campioni
		max = getMaxValues();
		min = getMinValues();
		diff = max - min;
		
		if(values.length>0)
		{
			setVerLabels(min, max);
		}
		// calcola il n. di divisioni degli assi
		nHorizontalDivisions = horlabels.length - 1;
		nVerticalDivisions = verlabels.length - 1;
	}
	
	// calcola l'altezza del valore
	private float CalcHeight(float value)
	{
		float val = value - min;
		float rat = val / diff;
		float h = graphheight * rat;
		return h;
	}
	
	// http://stackoverflow.com/questions/9743098/android-text-on-canvas
	
	
	private void ConfigureGraph()
	{
		//----------------------------------------
		// imposta i colori del grafico
		colorPoint = (Color.RED);		// colore del punto disegnato
		colorText = (Color.WHITE);		// colore del testo
		colorAxis = (Color.DKGRAY);		// colore delle linee degli assi
		
		//----------------------------------------
		// imposta le dimensioni del grafico
		//
		border = 20;
		/**
		originale
		if(verlabels.length > 0)
		{
			// se ci sono le label verticali
			horstart = border * 2;
		}
		else
		{
			// se non ci sono le label verticali
			horstart = border;
		}
		**/
		horstart = border * 2;
		if(horstart < maxWidthVerLabels)
		    horstart = maxWidthVerLabels;
		
		//----------------
		// disabled: height = getHeight();		// height of the current drawing layer
		// width = getWidth() - 1;		// width of the current drawing layer
		//----------------
		margin_Y = 20;
		if(margin_Y < textHeightPx)
			margin_Y = textHeightPx;
		// ori: graphheight = height - (2 * border);
		graphheight = height - (2 * margin_Y);
		
		margin_X = 20;
		if(margin_X < minWidthFree)
			margin_X = minWidthFree;
		// graphwidth = width - (2 * margin_X);
		// graphwidth = width - horstart - margin_X;
		graphwidth = width - horstart - minWidthFree;
		
		// larghezza della barra del grafico a barre
		if(datalength>1)
		{
			colwidth = graphwidth / (float)(datalength-1);
		}
		else
		{
			// nel caso il n. di campioni sia minore o uguale a 0
			colwidth = graphwidth;
		}
		//----------------------------------------
	}
	
	private void plotBarGraph(Canvas canvas) 
	{
		// grafico a barre
		// canvas.drawColor(Color.BLUE);
		// float colwidth = (width - (2 * border)) / datalength;
		for (int i = 0; i < values.length; i++) 
		{
			float h = CalcHeight(values[i]);
			float startX = (i * colwidth) + horstart;
			// float endX = startX + (colwidth - 1);
			float endX = startX + (colwidth);
			canvas.drawRect(
				startX, 
				(margin_Y - h) + graphheight, 
				endX, 
				height - (margin_Y - 1), 
				paint
				);
		}
	}

	private void plotHorizontalLine(Canvas canvas, float Yvalue, int color)
	{
		int svColor;

		float h = CalcHeight(Yvalue);
		
		svColor = paint.getColor();
		
		// linee orizzontali in corrispondenza alle label verticali
		paint.setColor(color);		// colore della linea
		// float y = margin_Y + (graphheight * delta );
		float y = (margin_Y - h) + graphheight;

		canvas.drawLine(horstart, y, width, y, paint);
		paint.setColor(svColor);
	}
	
	private void plotLineGraph(Canvas canvas) 
	{
		// float colwidth = (width - (2 * border)) / datalength;
		// canvas.drawColor(Color.BLUE);
		float halfcol = colwidth / 2;
		float lasth = 0;
		for (int i = 0; i < values.length; i++) 
		{
			float h = CalcHeight(values[i]);
			
			if (i > 0)
			{
				// canvas.drawLine(startx, starty, endx, endy, paint);
				canvas.drawLine(
					((i - 1) * colwidth) + (horstart + 1) + halfcol, 
					(margin_Y - lasth) + graphheight,
					(i * colwidth) + (horstart + 1) + halfcol,
					(margin_Y - h) + graphheight, 
					paint
					);
			}
			lasth = h;
		}
	}
	
	@Override
	protected void onDraw(Canvas canvas) 
	{
		SetValuesParameters();
		ConfigureGraph();

		// ------------------------------------------
		// stampa il titolo
		paint.setColor(colorText);		// colore del testo
		paint.setTextAlign(Align.CENTER);
		canvas.drawText(title, (graphwidth / 2) + horstart, margin_Y - 4, paint);
		
		// ------------------------------------------
		// stampa i riferimenti per l'asse verticale
		//
		paint.setTextAlign(Align.LEFT);
		if(verlabels.length > 0)
		{
			for (int i = 0; i < verlabels.length; i++) 
			{
				// linee orizzontali in corrispondenza alle label verticali
				paint.setColor(colorAxis);		// colore delle linee degli assi
				float y = margin_Y + ( (graphheight / nVerticalDivisions) * i );
				// canvas.drawLine(startx, starty, endx, endy, paint);
				canvas.drawLine(horstart, y, width, y, paint);

				// stampa le label verticali
				paint.setColor(colorText);		// colore del testo
				paint.setTextAlign(Align.RIGHT);
				// canvas.drawText(verlabels[i], 0, y, paint);
				canvas.drawText(verlabels[i], horstart - 2, y, paint);
			}
		}
		else
		{
			// stampa l'asse X
			paint.setColor(colorAxis);		// colore delle linee degli assi
			float y = margin_Y + graphheight;
			// canvas.drawLine(startx, starty, endx, endy, paint);
			canvas.drawLine(horstart, y, width, y, paint);
		}
		
		// ------------------------------------------
		// stampa i riferimenti per l'asse orizzontale
		//
		// calcola il delta delle divisioni sull'asse X
		float divisionsDeltaX = graphwidth / ((float)nHorizontalDivisions);
		//
		for (int i = 0; i < horlabels.length; i++) 
		{
			// linee verticali in corrispondenza alle label orizzontali
			paint.setColor(colorAxis);		// colore delle linee degli assi
			// stile della linea
			// paint.setStyle(Paint.Style.FILL_AND_STROKE);
			// stampa delle linee verticali corrispondenti alla label
			float x = horstart + ( divisionsDeltaX * (float)i );
			// canvas.drawLine(startx, starty, endx, endy, paint);
			canvas.drawLine(x, height - margin_Y, x, margin_Y, paint);

			// label orizzontali
			paint.setColor(colorText);		// colore del testo
			// imposta l'allineamento del testo della label orizzontale
			if (i == 0)
				paint.setTextAlign(Align.LEFT);
			else if (i == horlabels.length - 1)
				paint.setTextAlign(Align.RIGHT);
			else
				paint.setTextAlign(Align.CENTER);
			// stampa la label
			canvas.drawText(horlabels[i], x, height - 4, paint);
		}
		// restore left alignment
		paint.setTextAlign(Align.LEFT);

		// ------------------------------------------
		// disegna il grafico
		//
		if (max != min) 
		{
			// paint.setColor(Color.LTGRAY);
			// paint.setColor(Color.RED);
			paint.setColor(colorPoint);
			
			if (type == BAR) 
			{
				plotBarGraph(canvas);
				plotHorizontalLine(canvas, -100, Color.YELLOW);
			} 
			else 
			{
				// grafico normale
				plotLineGraph(canvas);
			}
		}
	}

	// get the max value to draw
	private float getMaxValues() {
		float largest = Integer.MIN_VALUE;
		for (int i = 0; i < values.length; i++)
			if (values[i] > largest)
				largest = values[i];
		return largest;
	}

	// get the min value to draw
	private float getMinValues() {
		float smallest = Integer.MAX_VALUE;
		for (int i = 0; i < values.length; i++)
			if (values[i] < smallest)
				smallest = values[i];
		return smallest;
	}
}
