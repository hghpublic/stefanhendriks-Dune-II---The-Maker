/* 

  Dune II - The Maker

  Author : Stefan Hendriks
  Contact: stefan@fundynamic.com
  Website: http://dune2themaker.fundynamic.com

  2001 - 2021 (c) code by Stefan Hendriks

  */

class cParticle {
public:

    // used or not:
    bool bAlive;        // alive

    // Drawing effects
    long x, y;              // x and y position to draw (absolute numbers). This is the 'center' of a particle, not top left!
    int iType;              // type
    sBitmap * bmp;          // pointer to bitmap to draw (not owner of bitmap!)
    int iAlpha;             // alpha

    int frameIndex;         // frame to draw

    // TIMERS
    int TIMER_frame;    // frame animation timers (when < 0, next frame, etc)
    // when TIMER_dead < 0, the last frame lets this thing die

    int TIMER_dead;     // when > -1, this timer will determine when the thing dies

    int layer;          // on which layer to draw

    int iHousePal;      // when specified, use this palette for drawing (and its an 8 bit picture then!)

    // ---------
    int draw_x();

    int draw_y();

    void draw();

    bool isValid();

    void init();
    void init(const s_ParticleInfo &particleInfo);

    void think();       // particles do some sort of thinking , really! :)

    static void create(long x, long y, int iType, int iHouse, int iFrame);
    static int findNewSlot();

    bool isUsingAlphaChannel() const;

private:
    int getFrameWidth();
    int getFrameHeight();

    s_ParticleInfo getParticleInfo() const;
};

void PARTICLE_CREATE(long x, long y, int iType, int iHouse, int iFrame);


